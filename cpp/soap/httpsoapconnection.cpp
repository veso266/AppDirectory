/***************************************************************************
                          httpsoapconnection.cpp -  description
                             -------------------
    begin                : Sun Sep 25 2005
    copyright            : (C) 2005 by Diederik van der Boor
    email                : vdboor --at-- codingdomain.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "httpsoapconnection.h"

#include "../../utils/kmessshared.h"
#include "../../utils/xmlfunctions.h"
#include "../../kmessapplication.h"
#include "../../kmessdebug.h"
#include "../mimemessage.h"
#include "soapmessage.h"
#include "config-kmess.h"

#include <QAuthenticator>
#include <QMutexLocker>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslError>

#include <KLocale>


#ifdef KMESSDEBUG_HTTPSOAPCONNECTION
  #define KMESSDEBUG_HTTPSOAPCONNECTION_GENERAL
  #define KMESSDEBUG_HTTPSOAPCONNECTION_HTTPDUMP
#endif


/**
 * Maximum allowed time between a request and its response
 *
 * KMess will wait at most this number of milliseconds after sending a request.
 * If no response arrives and this timeout expires, an error signal will be
 * emitted.
 */
#define SOAPCONNECTION_RESPONSE_TIMEOUT     60000



/**
 * @brief The constructor
 *
 * Initializes the class.
 * The actual connection is established when a SOAP call is made.
 *
 * @param  parent    The Qt parent object. The object is deleted when the parent is deleted.
 */
HttpSoapConnection::HttpSoapConnection( QObject *parent )
: QObject( parent )
, currentRequest_( 0 )
, isSending_( false )
{
#ifdef KMESSDEBUG_HTTPSOAPCONNECTION_GENERAL
  kDebug() << "CREATED.";
#endif

  // Create the network request object
  http_ = new QNetworkAccessManager( this );

  // Make signal connections
  connect( http_, SIGNAL(                   finished(QNetworkReply*)                         ),
           this,  SLOT  (        slotRequestFinished(QNetworkReply*)                         ) );
  connect( http_, SIGNAL(     authenticationRequired(QNetworkReply*,QAuthenticator*)         ),
           this,  SLOT  ( slotAuthenticationRequired(QNetworkReply*,QAuthenticator*)         ) );
  connect( http_, SIGNAL(                  sslErrors(QNetworkReply*,const QList<QSslError>&) ),
           this,  SLOT  (              slotSslErrors(QNetworkReply*,const QList<QSslError>&) ) );

  // Initialize the timeout timer
  responseTimer_.setSingleShot( true );
  responseTimer_.setInterval( SOAPCONNECTION_RESPONSE_TIMEOUT );
  connect( &responseTimer_, SIGNAL(            timeout() ),
            this,           SLOT  ( slotRequestTimeout() ) );
}



/**
 * @brief The destructor.
 *
 * Closes the sockets if these are still open.
 */
HttpSoapConnection::~HttpSoapConnection()
{
  qDeleteAll( requests_ );
  delete currentRequest_;
  delete http_;

#ifdef KMESSDEBUG_HTTPSOAPCONNECTION_GENERAL
  kDebug() << "DESTROYED.";
#endif
}



/**
 * @brief Abort all queued requests
 *
 * This removes any pending requests and ignores responses for already sent messages.
 */
void HttpSoapConnection::abort()
{
  qDeleteAll( requests_ );
  requests_.clear();

  delete currentRequest_;
  currentRequest_ = 0;

  responseTimer_.stop();
}



/**
 * @brief Return the current request message, if any
 *
 * It only returns a message if a request is currently being sent, or if its response is being processed.
 *
 * @param   copy  If true, creates a copy of the request message.
 * @return  Returns a message if one is being sent or processed, otherwise returns 0.
 */
SoapMessage *HttpSoapConnection::getCurrentRequest( bool copy ) const
{
  if( copy )
  {
    return new SoapMessage( *currentRequest_ );
  }
  else
  {
    return currentRequest_;
  }
}



/**
 * @brief Return whether the connection is idle.
 * @return  Returns true when the connection is idle, false when a SOAP request is pending / being processed.
 */
bool HttpSoapConnection::isIdle()
{
  return ( ! isSending_ );
}


#if 0
/**
 * @brief Parse the received server response.
 *
 * This body extracts the XML message from the body.
 * It detects SOAP Faults automatically.
 * Other responses are returned to parseSoapResult().
 *
 * @param   responseBody  The full message body received from the HTTP server.
 * @return  Whether there is a parsing error that should be emitted.
 *
 */
bool HttpSoapConnection::parseHttpBody( const QByteArray &responseBody )
{
#ifdef KMESSDEBUG_HTTPSOAPCONNECTION_GENERAL
  kDebug() << "Request finished, pasing body.";
#endif

  // Attempt to convert to XML
  // see http://doc.trolltech.com/4.3/qdomdocument.html#setContent
  QString xmlError;
  QDomDocument xml;
  if( ! xml.setContent( responseBody, true, &xmlError ) )   // true for namespace processing.
  {
    kWarning() << "Unable to parse XML "
                  "(error='" << xmlError << "' endpoint=" << endpoint_ << ")" << endl;
    return false;
  }

  // See if there is a header element
  QDomElement headerNode = XmlFunctions::getNode(xml, "/Envelope/Header").firstChild().toElement();

  // See if there is a Fault in the main Envelope (happens with passport RST service).
  QDomElement faultNode = XmlFunctions::getNode(xml, "/Envelope/Fault").toElement();
  if( ! faultNode.isNull() )
  {
#ifdef KMESSDEBUG_HTTPSOAPCONNECTION_GENERAL
    kDebug() << "Found soap fault outside the body!";
#endif
    closeConnection();
    parseSoapFault( faultNode, headerNode );
    return true;   // no parsing error, it's a valid response.
  }

  // Get the main body node
  QDomElement resultNode = XmlFunctions::getNode(xml, "/Envelope/Body").firstChild().toElement();
  if( resultNode.isNull() )
  {
    kWarning() << "Unable to parse response: result node is null "
                << "(root=" << xml.nodeName()  << " endpoint=" << endpoint_ << ")." << endl;
    return false;
  }

  // See if it's a soap error message, fault in the body.
  // NOTE: the test is done with localName() instead of nodeName(), because the prefix may differ.
  if( resultNode.localName() == "Fault"
  &&  resultNode.namespaceURI() == XMLNS_SOAP )
  {
#ifdef KMESSDEBUG_HTTPSOAPCONNECTION_GENERAL
    kDebug() << "Found soap fault.";
#endif
    closeConnection();
    parseSoapFault( resultNode, headerNode );
    return true;   // no parsing error, it's a valid response.
  }

  // WS-I compliant webservices wrap the response in another object so the response has one root element.
  // So you get a <MethodNameResponse><MethodNameResult>...</MethodNameResult></MethodNameResponse>
  // This object wrapper is stripped transparently in .Net, also do this here:
  QString resultNodeName( resultNode.nodeName() );
  if( resultNodeName.endsWith("Response")
  &&  resultNode.childNodes().length()   == 1
  &&  resultNode.firstChild().nodeName() == resultNodeName.replace("Response", "Result") )
  {
    resultNode = resultNode.firstChild().toElement();
  }

  // Request completed, indicate derived class and listeners.
#ifdef KMESSDEBUG_HTTPSOAPCONNECTION_GENERAL
  kDebug() << "Request finished, signalling to start parsing of the result node "
              "(name:"      << resultNode.nodeName()     <<
              " prefix:"    << resultNode.prefix()       <<
              " localName:" << resultNode.localName()    <<
              " namespace:" << resultNode.namespaceURI() << ")" << endl;
#endif

  // Notify listeners
  parseSoapResult( resultNode, headerNode );
  return true;
}
#endif



/**
 * @brief  Parse the SOAP fault.
 *
 * This method extracts the node data, and emits a soapError() signal.
 * Overwrite this method to implement custom error handling.
 *
 * @param  message   The message containing a soap fault conXML node which contains the SOAP fault.
 */
void HttpSoapConnection::parseSoapFault( SoapMessage *message )
{
  // Assemble a generic error message
  emit soapError( QString( "%1 (%2)" ).arg( message->getFaultDescription() )
                                      .arg( message->getFaultCode() ),
                  MsnSocketBase::ERROR_SOAP_UNKNOWN );
}



/**
 * @brief  Send the next request in queue to the endpoint.
 */
void HttpSoapConnection::sendNextRequest()
{
  if( isSending_ || requests_.isEmpty() )
  {
    return;
  }

#ifdef KMESSTEST
  KMESS_ASSERT( currentRequest_ == 0 );
#endif

  // We're sending from now on, block further sending attempts
  isSending_ = true;

  // Get the request to send
  currentRequest_ = requests_.takeFirst();

  // Verify if the request we're sending is valid
  if( ! currentRequest_->isValid() )
  {
    // Inform listeners that the request could not be sent (and disconnect)
    emit soapError( i18nc( "Error message (system-generated description)",
                           "Invalid web service request (%1)", currentRequest_->getFaultDescription() ),
                    MsnSocketBase::ERROR_INTERNAL );

    // Reset the internal data
    delete currentRequest_;
    currentRequest_ = 0;
    isSending_ = false;
    return;
  }

  const QString  &endpointAddress = currentRequest_->getEndPoint();

  QNetworkRequest request;
  QUrl            endpoint( endpointAddress );
  QByteArray      contents( currentRequest_->getMessage() );
  QString         soapAction( currentRequest_->getAction() );

  // Transparently handle host redirections
  if( redirections_.contains( endpoint.host() ) )
  {
    endpoint.setHost( redirections_[ endpoint.host() ] );

#ifdef KMESSDEBUG_HTTPSOAPCONNECTION_GENERAL
    kDebug() << "Sending with redirection from:" << endpointAddress << "to:" << endpoint;
#endif
  }


#ifdef KMESSTEST
  KMESS_ASSERT( endpoint.isValid() );
  KMESS_ASSERT( ( endpoint.scheme() == "http" ) || ( endpoint.scheme() == "https" ) );
#endif

  // Set the request header
  request.setUrl      ( endpoint                                                         );
  request.setHeader   ( QNetworkRequest::ContentTypeHeader,   "text/xml; charset=utf-8"  );
  request.setHeader   ( QNetworkRequest::ContentLengthHeader, contents.size()            );
  request.setRawHeader( "Host",                               endpoint.host().toLatin1() );
  request.setRawHeader( "Accept",                             "*/*"                      );
  request.setRawHeader( "User-Agent",                         "KMess/" KMESS_VERSION     );
  request.setRawHeader( "Connection",                         "Keep-Alive"               );
  request.setRawHeader( "Cache-Control",                      "no-cache"                 );

#ifdef KMESSTEST
  // Redirect to another server if started with --server
  if( static_cast<KMessApplication*>( kapp )->getUseTestServer() )
  {
    endpoint.setHost( static_cast<KMessApplication*>( kapp )->getTestServer() );
    endpoint.setPort( 4430 );
    request.setUrl( endpoint );
  }
#endif

  if( ! soapAction.isNull() )
  {
    QString quotedAction( "\"" + soapAction + "\"" );
    request.setRawHeader( "SOAPAction", quotedAction.toLatin1() );
  }

  http_->post( request, contents );

  // Start the response timer
  responseTimer_.start();

#ifdef KMESS_NETWORK_WINDOW
  QUrl soapActionUrl( soapAction );
  if( ! soapAction.isEmpty() && soapActionUrl.isValid() )
  {
    KMESS_NET_INIT( this, "SOAP " + soapActionUrl.path() );
  }
  else
  {
    KMESS_NET_INIT( this, "SOAP " + endpoint.path()      );
  }

  KMESS_NET_SENT( this, contents );
#endif

#ifdef KMESSDEBUG_HTTPSOAPCONNECTION_HTTPDUMP
  kDebug() << "Request contents:";
  kDebug() << contents;
#endif
}



/**
 * @brief Send a SOAP request to the webservice.
 *
 * This method adds a message to the sending queue.
 * If the urgent parameter is true, the request is sent before all
 * the other messages.
 *
 * @param  message  The message to send.
 * @param  urgent   Whether the request needs to be sent before everything else.
 */
void HttpSoapConnection::sendRequest( SoapMessage *message, bool urgent )
{
#ifdef KMESSDEBUG_HTTPSOAPCONNECTION_GENERAL
  if( urgent )
  {
    kDebug() << "Immediately sending message to endpoint:" << message->getEndPoint();
  }
  else
  {
    kDebug() << "Queueing message to endpoint:" << message->getEndPoint();
  }
#endif

  if( urgent )
  {
    requests_.prepend( message );
  }
  else
  {
    requests_.append( message );
  }

  // Try sending the request now
  sendNextRequest();
}



/**
 * @brief Called when the remote server requires authentication.
 */
void HttpSoapConnection::slotAuthenticationRequired( QNetworkReply *reply, QAuthenticator *authenticator )
{
  // A response has arrived, stop the timeout detection timer
  responseTimer_.stop();

  kWarning() << "Got http authentication request for" << authenticator->realm() <<
                "while connecting to" << reply->url();

  // Inform listeners that the request failed
  emit soapError( i18nc( "Error message (system-generated description)",
                         "The web service is not accessible (%1)",
                         reply->errorString() ),
                  MsnSocketBase::ERROR_SOAP_AUTHENTICATION );
}



// The request to the remote server finished
void HttpSoapConnection::slotRequestFinished( QNetworkReply *reply )
{
  QMutexLocker locker( &lockMutex_ );

  // An unexpected response has arrived
  if( currentRequest_ == 0 )
  {
    kWarning() << "No request in progress!";
    return;
  }

  // A response has arrived, stop the timeout detection timer
  responseTimer_.stop();

  const QByteArray& replyContents = reply->readAll();
  const int         statusCode    = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute   ).toInt();
  const QString     error         ( reply->attribute( QNetworkRequest::HttpReasonPhraseAttribute ).toString() );

#ifdef KMESSDEBUG_HTTPSOAPCONNECTION_GENERAL
  bool requestSuccess = false;
  const QUrl replyUrl( reply->url() );
  kDebug() << "Received response: code" << statusCode
           << reply->attribute( QNetworkRequest::HttpReasonPhraseAttribute )
           << " from" << replyUrl;
#endif
#ifdef KMESSDEBUG_HTTPSOAPCONNECTION_HTTPDUMP
  kDebug() << "Reply contents:";

  // Create the document with the content of SOAP reply to indent it
  QDomDocument document;
  document.setContent( replyContents );
  kDebug() << document.toString();
#endif

  // Parse the body of the HTTP response (copying the other attributes from the request)
  SoapMessage *currentResponse = getCurrentRequest( true /* copy */ );
  currentResponse->setMessage( replyContents );

#ifdef KMESS_NETWORK_WINDOW
    KMESS_NET_RECEIVED( this, replyContents );
#endif

  // Check for errors
  bool success = currentResponse->isValid();
  if( success )
  {
#ifdef KMESSDEBUG_HTTPSOAPCONNECTION_GENERAL
    requestSuccess = true;
#endif

    // Parse the message contents
    if( currentResponse->isFaultMessage() )
    {
      // Verify if the server is redirecting us to another server
      if( currentResponse->getFaultCode() == "psf:Redirect" )
      {
        const QUrl& originalUrl = currentResponse->getEndPoint();
        const QUrl redirectUrl( XmlFunctions::getNodeValue( currentResponse->getFault(), "redirectUrl" ) );
        const QString originalHost( originalUrl.host() );
        const QString redirectHost( redirectUrl.host() );

#ifdef KMESSDEBUG_HTTPSOAPCONNECTION_GENERAL
        kDebug() << "Received redirection from:" << originalHost << "to:" << redirectHost;
#endif

        // Save the redirection target
        if( ! redirectionCounts_.contains( originalHost ) )
        {
          redirectionCounts_[ originalHost ] = 0;
        }
        redirections_[ originalHost ] = redirectHost;
        redirectionCounts_[ originalHost ] = redirectionCounts_[ originalHost ] + 1;

        // Limit the number of redirects from the original host
        if( redirectionCounts_[ originalHost ] > 5 )
        {
          emit soapError( i18nc( "Error message", "Too many redirections by web service" ),
                          MsnSocketBase::ERROR_SOAP_TOOMANYREDIRECTS );
        }
        else
        {
          // Retry sending the message by sending an identical one immediately.
          // We've mapped the old endpoint to the new one so there's no need to change the message
          // endpoint. Also, we copy it because this method deletes the current message.
          SoapMessage *messageCopy = new SoapMessage( *currentRequest_ );
          requests_.prepend( messageCopy );
        }
      }
      else
      {
        parseSoapFault( currentResponse );
      }
    }
    else
    {
      const QUrl originalUrl( currentResponse->getEndPoint() );
      const QString originalHost( originalUrl.host() );
      QString preferredHostName( XmlFunctions::getNodeValue( currentResponse->getHeader(),
                                                             "ServiceHeader/PreferredHostName" ) );

      // Verify if the server is suggesting us to use another server
      if( ! preferredHostName.isEmpty() && ! redirections_.contains( originalHost ) )
      {
#ifdef KMESSDEBUG_HTTPSOAPCONNECTION_GENERAL
        kDebug() << "Received hostname suggestion from:" << originalUrl << "to:" << preferredHostName;
#endif

        redirectionCounts_[ originalHost ] = 0;
        redirections_[ originalHost ] = preferredHostName;
      }

      // Then parse this response
      parseSoapResult( currentResponse );
    }
  }
  else if( statusCode == 503 )
  {
    if( metaObject()->className() == QString( "OfflineImService" ) )
    {
      emit soapWarning( i18nc( "Warning message",
                               "The Offline Messages web service is currently not available" ),
                        false );
    }
    else
    {
      emit soapWarning( i18nc( "Warning message",
                               "The Live Messenger web service is experiencing problems" ),
                        false );
    }
  }
  else
  {
    // Display at the console for logging
    kWarning() << "Received an unknown" << statusCode << "status code (" << error
               << ") while connecting to endpoint" << reply->url();
    kWarning() << "Reply contents:";
    kWarning() << replyContents;

    // Inform listeners that the request failed
    emit soapError( i18nc( "Error message with description (system-generated description)",
                           "Invalid web service response %1 (%2)",
                           error,
                           currentResponse->getFaultDescription() ),
                    MsnSocketBase::ERROR_SOAP_RESPONSE );
  }

  // Terminate the reply
  reply->abort();
  reply->deleteLater();

  // Reset all internal data
  delete currentRequest_;
  delete currentResponse;
  currentRequest_ = 0;
  isSending_ = false;

#ifdef KMESSDEBUG_HTTPSOAPCONNECTION_GENERAL
  kDebug() << "Completed response handling from endpoint" << replyUrl << ", with success?" << requestSuccess;
#endif

  // Send the next queued message
#if QT_VERSION > 0x040500
  // Work around a Qt-4.5.1-devel bug which doesn't clean up correctly after each request.
  QTimer::singleShot( 250, this, SLOT(sendNextRequest()));
#else
  sendNextRequest();
#endif
  return;
}



/**
 * @brief Called when a timeout occurred while sending a request.
 */
void HttpSoapConnection::slotRequestTimeout()
{
  // Inform listeners that the request failed
  emit soapError( i18nc( "Error message",
                         "No response from web service" ),
                  MsnSocketBase::ERROR_SOAP_TIME_LIMIT );
}



/**
 * @brief Called when an ssl error occurred while setting up the connection.
 */
void HttpSoapConnection::slotSslErrors( QNetworkReply *reply, const QList<QSslError> &sslErrors )
{
  foreach( QSslError error, sslErrors )
  {
    kWarning() << "Got SSL error" << error.errorString() << "while connecting to endpoint" << reply->url();
  }

  // It's needed to ignore SSL errors because rsi.hotmail.com uses an invalid certificate.
  reply->ignoreSslErrors();
}



/**
 * @brief Decode UTF-8 text from a SOAP node (usually friendly names).
 */
QString HttpSoapConnection::textNodeDecode( const QString &string )
{
  QString copy( QString::fromUtf8( string.toLatin1() ) );
  return KMessShared::htmlUnescape( copy );
}



#include "httpsoapconnection.moc"
