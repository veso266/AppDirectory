/***************************************************************************
                          offlineimservice.cpp -  description
                             -------------------
    begin                : Thu Sep 07 2006
    copyright            : (C) 2006 by Diederik van der Boor
    email                : "vdboor" --at-- "codingdomain.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "offlineimservice.h"

#include "../../utils/xmlfunctions.h"
#include "../../utils/kmessshared.h"
#include "../../currentaccount.h"
#include "../../kmessdebug.h"
#include "../applications/p2papplication.h"
#include "../chatmessage.h"
#include "../mimemessage.h"
#include "../msnchallengehandler.h"
#include "soapmessage.h"

#include <QTextDocument>

#include <KDateTime>


#ifdef KMESSDEBUG_OFFLINE_IM
#define KMESSDEBUG_OFFLINE_IM_GENERAL
#endif



/**
 * @brief URL of the Offline IM Receiving Service
 */
#define SERVICE_URL_INCOMING_OFFLINE_IM_SERVICE  "https://rsi.hotmail.com/rsi/rsi.asmx"

/**
 * @brief URL of the Offline IM Sending Service
 */
#define SERVICE_URL_OUTGOING_OFFLINE_IM_SERVICE  "https://ows.messenger.msn.com/OimWS/oim.asmx"





/**
 * @brief  The receiving mode constructor.
 *
 * Initializes the client to receive offline messages using the Offline-IM webservice.
 *
 * @param  authT   The <code>t</code> value of the passport cookie.
 * @param  authP   The <code>p</code> value of the passport cookie.
 * @param  parent  The Qt parent object, when it's destroyed this class will be cleaned up automatically too.
 */
OfflineImService::OfflineImService( const QString &authT, const QString &authP,
                                     QObject *parent )
: PassportLoginService( parent )
, authT_(authT)
, authP_(authP)
, nextSequenceNum_(0)
{
#ifdef KMESSDEBUG_OFFLINE_IM_GENERAL
  kDebug() << "CREATED offline IM receiver service";
#endif

  setObjectName( "OfflineImService[receiver]" );

  // Initialize the header once. The tokens are escaped after getting copied (mid(0) returns a copy)
  passportCookieHeader_ =
      "    <PassportCookie xmlns=\"http://www.hotmail.msn.com/ws/2004/09/oim/rsi\">\n"
      "      <t>" + KMessShared::htmlEscape( authT.mid( 0 ) ) + "</t>\n"
      "      <p>" + KMessShared::htmlEscape( authP.mid( 0 ) ) + "</p>\n"
      "    </PassportCookie>";
}



/**
 * @brief  The sending mode constructor.
 *
 * Initializes the client to send offline messages using the Offline-IM webservice.
 *
 * @param  parent  The Qt parent object, when it's destroyed this class will be cleaned up automatically too.
 */
OfflineImService::OfflineImService( QObject *parent )
  : PassportLoginService( parent )
  , nextSequenceNum_(0)
{
#ifdef KMESSDEBUG_OFFLINE_IM_GENERAL
  kDebug() << "CREATED offline IM sender service";
#endif

  setObjectName( "OfflineImService[sender]" );
}



/**
 * @brief  The destructor.
 */
OfflineImService::~OfflineImService()
{
#ifdef KMESSDEBUG_OFFLINE_IM_GENERAL
  kDebug() << "DESTROYED.";
#endif
}



/**
 * @brief  SOAP call to delete messages from the remote storage.
 * @param  messageIds  List of messages to delete.
 */
void OfflineImService::deleteMessages( const QStringList &messageIds )
{
#ifdef KMESSDEBUG_OFFLINE_IM_GENERAL
  kDebug() << "requesting deletion of messages:" << messageIds;
#endif
#ifdef KMESSTEST
  KMESS_ASSERT( ! messageIds.isEmpty() );
#endif

  QString body( "    <DeleteMessages xmlns=\"http://www.hotmail.msn.com/ws/2004/09/oim/rsi\">\n"
                "      <messageIds>\n"
                "        <messageId>" + messageIds.join("</messageId>\n        <messageId>") + "</messageId>\n"
                "      </messageIds>\n"
                "    </DeleteMessages>" );

  sendSecureRequest( new SoapMessage( SERVICE_URL_INCOMING_OFFLINE_IM_SERVICE,
                                      "http://www.hotmail.msn.com/ws/2004/09/oim/rsi/DeleteMessages",
                                      passportCookieHeader_,
                                      body ) );
}



/**
 * @brief Internal function to extract the email address from an RFC822 formatted string.
 *
 * For a value such as '<code>"Contactname" &lt;contact@hotmail.com&gt;</code>',
 * this method returns '<code>contact@hotmail.com</code>'.
 *
 * @param    address  The string with an email address.
 * @returns  The email address of the contact.
 */
QString OfflineImService::extractRFC822Address( const QString &address )
{
  if( address.endsWith(">") )
  {
    int pos = address.lastIndexOf('<');
    if( pos != -1 )
    {
      return address.mid( pos + 1, address.length() - pos - 2 );
    }
  }

  return address;
}



/**
 * @brief SOAP call to download an offline message.
 *
 * The message ID can be extracted from
 * the following messages types received at the notification connection:
 * - <code>text/x-msmsgsoimnotification</code>
 * - <code>text/x-msmsgsinitialmdatanotification</code>
 *
 * The messageReceived() signal is fired when the webservice returns the message.
 *
 * @param  messageId  The ID of the message.
 * @param  markAsRead  Whether the message should be marked as read.
 */
void OfflineImService::getMessage( const QString &messageId, bool markAsRead )
{
#ifdef KMESSDEBUG_OFFLINE_IM_GENERAL
  kDebug() << "requesting message with ID:" << messageId;
#endif

  // Initialize the call data, which will be used in the slots to know which session this was
  MessageData data;
  data.type  = "GetMessage";
  data.value = messageId;

  // Initialize request
  QString soapBody(
      "    <GetMessage xmlns=\"http://www.hotmail.msn.com/ws/2004/09/oim/rsi\">\n"
      "      <messageId>" + KMessShared::htmlEscape( messageId ) + "</messageId>\n"
      "      <alsoMarkAsRead>" + ( markAsRead ? "true" : "false" ) + "</alsoMarkAsRead>\n"
      "    </GetMessage>" );

  // Send the request.
  sendRequest( new SoapMessage( SERVICE_URL_INCOMING_OFFLINE_IM_SERVICE,
                                "http://www.hotmail.msn.com/ws/2004/09/oim/rsi/GetMessage",
                                passportCookieHeader_,
                                soapBody,
                                data ) );
}



/**
 * @brief SOAP call to download the value of the Mail-Data field.
 *
 * The value of the <code>Mail-Data</code> field is normally received by the notification server.
 * If there are many offline-IM messages, the value of this field is literally <code>too-large</code>.
 * In that case, the message data can be downloaded using this method.
 *
 * The metaDataReceived() signal is fired when the webservice returns the data.
 */
void OfflineImService::getMetaData()
{
#ifdef KMESSDEBUG_OFFLINE_IM_GENERAL
  kDebug() << "requesting Mail-Data field";
#endif

  // Initialize the call data, which will be used in the slots to know which session this was
  MessageData data;
  data.type = "GetMetaData";

  // Send the request.
  sendRequest( new SoapMessage( SERVICE_URL_INCOMING_OFFLINE_IM_SERVICE,
                                "http://www.hotmail.msn.com/ws/2004/09/oim/rsi/GetMetadata",
                                passportCookieHeader_,
                                "<GetMetadata xmlns=\"http://www.hotmail.msn.com/ws/2004/09/oim/rsi\" />",
                                data ) );
}



/**
 * @brief Process the SOAP fault returned when sending an offline message.
 */
void OfflineImService::parseSecureFault( SoapMessage *message )
{
  // Should never happen
  if( ! message->isFaultMessage() )
  {
    kWarning() << "Incorrect fault detected on incoming message";
    return;
  }

  // Get the type of the error
  QString faultCode( message->getFaultCode() );

  // Get the message data
  MessageData messageData( message->getData()               );
  QStringList info       ( messageData.value.toStringList() );
  QString     recipient  ( info.value( 0 )                  );
  QString     contents   ( info.value( 1 )                  );
  int         sequenceNum( info.value( 2 ).toInt()          );

  // Get the OIM mime message, and replace its Base64-encoded body with the original message,
  // in case we need an usable mime message for error reporting.
  MimeMessage originalMessage( XmlFunctions::getNodeValue( getCurrentRequest()->getBody(), "Content" ) );
  originalMessage.setBody( contents );

  // See which fault we received
  if( faultCode == "q0:AuthenticationFailed" )
  {
    QDomNode faultNode( message->getFault() );
    QString lockKeyChallenge( XmlFunctions::getNodeValue( faultNode, "detail/LockKeyChallenge" ) );
    QString tweenerChallenge( XmlFunctions::getNodeValue( faultNode, "detail/TweenerChallenge" ) );

    // See if a lock key challenge was requested.
    if( ! lockKeyChallenge.isEmpty() )
    {
      // In the response from server there is a LockKeyChallenge that allows
      // to calculate the LockKey hash (together the Product Key and Product id of MSN).

      // Compute hash with MSN product key/id for the identification
      MSNChallengeHandler handler;

      // Compute the lock key using the MSNP11 Challenge algorithm
      currentAccount_->setOfflineImKey( handler.computeHash( lockKeyChallenge ) );

#ifdef KMESSDEBUG_OFFLINE_IM_GENERAL
      kDebug() << "Lock key to compute:" << lockKeyChallenge;
      kDebug() << "Computed lock key:"   << currentAccount_->getOfflineImKey();
#endif
      // Check if there is a TweenerChallenge, and if not re-send the message with the new lock key
      if( tweenerChallenge.isEmpty() )
      {
        storeMessage( recipient, contents, sequenceNum );
        return;
      }
    }

    // See if a tweener challenge was requested.
    if( ! tweenerChallenge.isEmpty() )
    {
      // Start the login attempt using the base class
      login( tweenerChallenge );
      return;
    }

    // Any other situation means something is wrong.
    kWarning() << "Unknown challenge type received";
    emit sendMessageFailed( recipient, originalMessage );
  }
  else if( faultCode == "q0:SystemUnavailable"              // contact does not exist OR has blocked you (unlikely)
       ||  faultCode == "q0:InvalidContent"                 // malformed request?
       ||  faultCode == "q0:DeliveryFailed"                 // happens with unverified passport accounts
       ||  faultCode == "q0:InvalidParameter"               // seen with SOAP syntax error
       ||  faultCode == "q0:SenderThrottleLimitExceeded" )  // TODO: Implement throttle control in HttpSoapConnection
  {
    // failed, can't send offline IM's
    emit sendMessageFailed( recipient, originalMessage );

    // Also report at the console.
    kWarning() << "Received SOAP fault" << faultCode << "when sending an offline-im message to" << recipient;
  }
  else
  {
    // Any other situation means something is wrong.
    kWarning() << "Unknown response type received: fault code:" << faultCode
               << "endpoint:" << message->getEndPoint();

    // Notify the sender if this error was
    // in response of a sent offline im.
    if( ! info.isEmpty() )
    {
      emit sendMessageFailed( recipient, originalMessage );
    }
  }
}



/**
 * @brief  Internal function to process the response of the webservice.
 *
 * This handles the following response nodes:
 * - <code>GetMessageResponse</code> is handled by processGetMessageResult().
 * - <code>GetMetadataResponse</code> is sent with the metaDataReceived() signal.
 * - <code>DeleteMessagesResponse</code> is ignored because it's always empty.
 *
 * Other unrecognized response types result in a warning message at the console.
 *
 * @param  message  The SOAP response message.
 */
void OfflineImService::parseSecureResult( SoapMessage *message )
{
#ifdef KMESSDEBUG_OFFLINE_IM_GENERAL
  kDebug() << "parsing SOAP response.";
#endif

  QDomElement body      ( message->getBody().toElement() );
  QString     resultName( body.firstChildElement().localName() );

  if( resultName == "GetMessageResponse" )
  {
    processGetMessageResult( message );
  }
  else if( resultName == "GetMetadataResponse" )
  {
    emit metaDataReceived( body.namedItem("MD").toElement() );
  }
  else if( resultName == "DeleteMessagesResponse" )
  {
    // do nothing
  }
  else if( resultName == "StoreResponse" )
  {
#ifdef KMESSDEBUG_OFFLINE_IM_GENERAL
    QStringList info     ( message->getData().value.toStringList() );
    QString     recipient( info.value( 0 ) );
    QString     contents ( info.value( 1 ) );

    kDebug() << "Stored offline message for" << recipient << ":" << contents;
#endif

    // Reset the session ID
    runID_ = QString();
  }
  else
  {
    kWarning() << "Unknown response type" << resultName << "received from endpoint" << message->getEndPoint();
  }
}



/**
 * @brief  Internal function to process the response of the getMessage() call.
 *
 * This method parses the XML data, extracts the relevant fields,
 * and fires the messageReceived() signal with the results.
 *
 * @param  message  The response message, containing a <code>GetMessageResponse</code> node.
 */
void OfflineImService::processGetMessageResult( SoapMessage *message )
{
#ifdef KMESSDEBUG_OFFLINE_IM_GENERAL
  kDebug() << "Parsing downloaded Offline-IM message.";
#endif

  // Retrieve the data stored in the SOAP message
  QString messageId( message->getData().value.toString() );

  // Parse as MIME message
  MimeMessage mimeMessage( message->getBody().toElement().text() );

  // Extract the fields
  QString from       ( mimeMessage.decodeRFC2047String( mimeMessage.getValue( "From" ).toUtf8() ) );
  QString to         ( mimeMessage.decodeRFC2047String( mimeMessage.getValue( "To"   ).toUtf8() ) );
  QString dateString ( mimeMessage.getValue( "Date"               ) );
  QString runId      ( mimeMessage.getValue( "X-OIM-Run-Id"       ) );
  int     sequenceNum( mimeMessage.getValue( "X-OIM-Sequence-Num" ).toInt() );

  // Convert the 'From' and 'To' lines, it's something like "Contactname <contact@hotmail.com>"
  from = extractRFC822Address( from );
  to   = extractRFC822Address( to );

  // Convert the date from RFC 2822 format, e.g. "15 Nov 2005 14:24:27 -0800"
  // NOTE: QDateTime ignores the timezone (and doesn't know about the RFC2822 format).
  // Also, QDateTime only knows about UTC and local time: we'll be able to compare dates with
  // precision only if we convert from KDateTime to local time or UTC!
  // Of course, KDateTime::dateTime() takes care of this.
  QDateTime date( KDateTime::fromString( dateString, KDateTime::RFCDate ).toClockTime().dateTime() );

  // Also check for invalid dates
  if( ! date.isValid() )
  {
    date = QDateTime::currentDateTime();
  }

  // Validate content type
  QString contentType( mimeMessage.getValue( "Content-Type" ) );
  if( contentType.toLower() != "text/plain; charset=utf-8" )
  {
    kWarning() << "Received unexpected content type:" << contentType;
  }

  // Validate transfer encoding
  QString transferEncoding( mimeMessage.getValue( "Content-Transfer-Encoding" ) );
  if( transferEncoding != "base64" )
  {
    kWarning() << "Received unexpected transfer encoding:" << transferEncoding;
  }

  // Validate message type
  QString messageType( mimeMessage.getValue( "X-OIM-Message-Type" ) );
  if( messageType != "OfflineMessage" )
  {
    kWarning() << "Received unexpected message type:" << messageType;
  }

  // Process the body
  QByteArray decodedBody( QByteArray::fromBase64( mimeMessage.getBody().toUtf8() ) );
  QString    body       ( QString::fromUtf8( decodedBody.data(), decodedBody.length() ) );

  // Return to caller
  emit messageReceived( messageId, from, to, date, body, runId, sequenceNum );
}



/**
 * @brief  Send an offline message.
 *
 * Sends an Offline Message. With the message the lockKey is empty, so the server will respond with a SOAP fault.
 * At parseSoapFault(), this error will be detected and the message is sent again with the proper lock key.
 *
 * @param  to       Handle of the message sender.
 * @param  message  Message body.
 */
void OfflineImService::sendMessage( const QString &to, const QString &message )
{
  // Increment sequence counter.
  ++nextSequenceNum_;

  // Call internal version with sequence number
  storeMessage( to, message, nextSequenceNum_ );
}



/**
 * @brief  Internal method to store a message in the offline-im storage.
 *
 * This method is used internally by sendMessage() and others to send the message.
 * It accepts a third sequenceNum parameter so retries are stored with the same sequence number.
 *
 * @param  to           Handle of the message sender.
 * @param  message      Message body.
 * @param  sequenceNum  The sequence number.
 */
void OfflineImService::storeMessage( const QString &to, const QString &message, int sequenceNum )
{
  MSNChallengeHandler handler;

  const QByteArray windowsNewLine( "\r\n" );
  const QString    from          ( currentAccount_->getHandle() );

  // Limit the friendly name to 48 chars to avoid the server doesn't accept the oim
  // The loop is necessary to avoid the cutting of one multi-byte character
  const QString originalFriendlyName( currentAccount_->getFriendlyName( STRING_CLEANED ) );
  QByteArray friendlyName;
  int size = 48;

  do
  {
    friendlyName = originalFriendlyName.left( size ).toUtf8();
    --size;
  }
  while( friendlyName.size() > 48 );

  const QString &offlineImKey = currentAccount_->getOfflineImKey();

  // Save the session data in the SOAP message, to allow slots to identify the source session
  MessageData data;
  data.type = "OIMStore";
  data.value = QStringList() << to << message << QString::number( sequenceNum );

  // Store the GUID for this sending session
  if( runID_.isEmpty() )
  {
    runID_ = KMessShared::generateGUID();
  }

  // Get a copy of the message with Windows linefeeds
  QString messageCopy( message );
  messageCopy.replace( QRegExp("\r?\n"), windowsNewLine );
  // Encode it to Base64
  QByteArray contentBody( messageCopy.toUtf8() );
  contentBody = contentBody.toBase64();

  // Insert newlines each 77 base64 chars.
  // NOTE: I know, this sounds pretty much unreasonable. But if we don't do it, the server
  // will answer with an HTTP/500 response and the SOAP fault code "q0:InvalidContent".
  // Is there a reason for this requirement?
  int contentBodySize = contentBody.size();
  if( contentBodySize > 77 )
  {
    for( int i = 76; i < contentBodySize; i += 77 )
    {
      contentBody.insert( i, windowsNewLine );
    }
  }

  // Build the request
  QString header( "<From xmlns=\"http://messenger.msn.com/ws/2004/09/oim/\""
                  " xml:lang=\"en-US\" proxy=\"MSNMSGR\" msnpVer=\"MSNP15\" buildVer=\"8.5.1302\""
                  " memberName=\"" + from + "\""
                  " friendlyName=\"=?utf-8?B?" + friendlyName.toBase64() + "?=\" />\n"
                  "<To xmlns=\"http://messenger.msn.com/ws/2004/09/oim/\" memberName=\"" + to + "\" />\n"
                  "<Ticket xmlns=\"http://messenger.msn.com/ws/2004/09/oim/\"\n"
                  " appid=\"" + handler.getProductId() + "\""
                  " lockkey=\"" + offlineImKey + "\""
                  " passport=\"\" />" // This attribute is filled by the base class
                  "<Sequence xmlns=\"http://schemas.xmlsoap.org/ws/2003/03/rm\">\n"
                  "  <Identifier xmlns=\"http://schemas.xmlsoap.org/ws/2002/07/utility\">http://messenger.msn.com</Identifier>\n"
                  "  <MessageNumber>" + QString::number( sequenceNum ) + "</MessageNumber>\n"
                  "</Sequence>" );

  QString body( "<MessageType xmlns=\"http://messenger.msn.com/ws/2004/09/oim/\">text</MessageType>\n"
                "<Content xmlns=\"http://messenger.msn.com/ws/2004/09/oim/\">"
                  "MIME-Version: 1.0\r\n"
                  "Content-Type: text/plain; charset=UTF-8\r\n"
                  "Content-Transfer-Encoding: base64\r\n"
                  "X-OIM-Message-Type: OfflineMessage\r\n"
                  "X-OIM-Run-Id: " + runID_ + "\r\n"
                  "X-OIM-Sequence-Num: " + QString::number( sequenceNum ) + "\r\n"
                  "\r\n" +
                  contentBody + "\r\n"
                "</Content>" );

  // Send the request
  sendSecureRequest( new SoapMessage( SERVICE_URL_OUTGOING_OFFLINE_IM_SERVICE,
                                      "http://messenger.live.com/ws/2006/09/oim/Store2",
                                      header,
                                      body,
                                      data ),
                     "MessengerSecure" );
}



#include "offlineimservice.moc"
