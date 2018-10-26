/***************************************************************************
                          passportloginservice.cpp  -  description
                             -------------------
    begin                : Sun Apr 22 2007
    copyright            : (C) 2007 by Diederik van der Boor
    email                : "vdboor" --at-- "codingdomain.com"
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "passportloginservice.h"

#include "../../utils/kmessshared.h"
#include "../../utils/xmlfunctions.h"
#include "../../currentaccount.h"
#include "../mimemessage.h"
#include "soapmessage.h"

#include <QUrl>

#include <KDateTime>
#include <KLocale>


/**
 * @brief URL of the Request Security Token Service
 */
#define SERVICE_URL_RST_SERVICE  "https://login.live.com/RST.srf"


// Static attributes initialization
QHash<QString,QDateTime> PassportLoginService::tokenExpirationDates_;



/*
 * Great documentation about this can be found at
 * http://msnpiki.msnfanatic.com/index.php/MSNP13:SOAPTweener
 */



/**
 * @brief The constructor
 *
 * Initializes the client to connect to the passport login (RST) service.
 */
PassportLoginService::PassportLoginService( QObject *parent )
: HttpSoapConnection( parent )
, currentAccount_( CurrentAccount::instance() )
, isWaitingForNewTokens_( false )
{
  // RST stands for "Request Security Token"
  // This class is called "PassportLoginService" since "RequestSecurityTokenService" is too hard to understand.

  setObjectName( "PassportLoginService" );
}



/**
 * @brief The destructor
 */
PassportLoginService::~PassportLoginService()
{
  qDeleteAll( queuedRequests_.keys() );
}



/**
 * @brief Start the login process
 *
 * The loginSucceeded() signal is fired when the webservice accepted the login data.
 * The loginIncorrect() signal is fired when the login data is incorrect.
 *
 * @param  parameters  The login parameters found in the <code>USR</code> command of notification server.
 */
void PassportLoginService::login( const QString &parameters )
{
#ifdef KMESSDEBUG_PASSPORTLOGINSERVICE
  kDebug() << "Logging in...";
#endif

  // Store the account and login data
  authenticationParameters_ = parameters;
  handle_                   = currentAccount_->getHandle();

  // Get the right password
  if( password_.isEmpty() )
  {
      // Try to login with the new password, if there is one
    if( ! currentAccount_->getTemporaryPassword().isEmpty() )
    {
      password_ = currentAccount_->getTemporaryPassword();
    }
    else
    {
      // Use the current password otherwise
      password_ = currentAccount_->getPassword();
    }
  }

#ifdef KMESSTEST
  KMESS_ASSERT( ! handle_  .isEmpty() );
  KMESS_ASSERT( ! password_.isEmpty() );
#endif

  // Send the login request
  requestMultipleSecurityTokens();
}



// Compute the string for live hotmail access
const QString PassportLoginService::createHotmailToken( const QString &passportToken,
                                                  const QString &proofToken,
                                                  const QString &folder )
{
  // Create the nonce
  QByteArray nonce;
  for( int i = 0; i < 24; i++ )
  {
    nonce += (char)( ( rand() % 74 ) + 48 );
  }

  // Create the percent encoded string
  const QString encodedString( QUrl::toPercentEncoding(
      "<EncryptedData xmlns=\"http://www.w3.org/2001/04/xmlenc#\""
      "Id=\"BinaryDAToken0\" Type=\"http://www.w3.org/2001/04/xmlenc#Element\">"
      " <EncryptionMethodAlgorithm=\"http://www.w3.org/2001/04/xmlenc#tripledes-cbc\"/>"
      " <ds:KeyInfo xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\">"
      "   <ds:KeyName>http://Passport.NET/STS</ds:KeyName>"
      " </ds:KeyInfo>"
      " <CipherData>"
      "   <CipherValue>" + passportToken + "</CipherValue>"
      " </CipherData>"
      "</EncryptedData>"
      , "", ".-" ) );

  // Create the token
  QString token( "ct="   + QString::number( QDateTime::currentDateTime().toTime_t(), 10 ) +
                 "&rru=" + QUrl::toPercentEncoding( folder ) +
                 "&sru=" + QUrl::toPercentEncoding( folder ) +
                 "&ru="  + QUrl::toPercentEncoding( folder ) +
                 "&bver=4&svc=mail&js=yes&id=2&pl=%3Fid%3D2&da="
                 + encodedString +
                 "&nonce=" + QUrl::toPercentEncoding( nonce.toBase64() ) );

  // Compute the keys with HMAC-Sha1 algorithm
  const QByteArray key1( QByteArray::fromBase64( proofToken.toLatin1() ) );

  const QByteArray magic( "WS-SecureConversation" + nonce );

  const QByteArray key2( KMessShared::deriveKey( key1, magic ) );

  const QByteArray hash( KMessShared::createHMACSha1( key2, token.toLatin1() ) );

  token += "&hash=" + QUrl::toPercentEncoding( hash.toBase64() );

  return token;
}



/**
 * @brief Bounce the authenticated SOAP fault to a subclass.
 * @internal
 *
 * This method receives SOAP fault messages which are not related to
 * authentication and analyzes them. It can be overridden by child classes
 * to allow them to obtain their own content.
 *
 * @param message  The SOAP response message.
 */
void PassportLoginService::parseSecureFault( SoapMessage *message )
{
  HttpSoapConnection::parseSoapFault( message );
}



/**
 * @brief Bounce the authenticated SOAP response to a subclass
 * @internal
 *
 * This method receives SOAP result messages which are not related to
 * authentication and analyzes them. It can be overridden by child classes
 * to allow them to obtain their own content.
 *
 * @param message  The SOAP response message.
 */
void PassportLoginService::parseSecureResult( SoapMessage *message )
{
  const QString resultType( message->getBody().firstChildElement().nodeName() );

  kWarning() << "Could not parse response type" << resultType
              << "from endpoint" << message->getEndPoint();

  emit soapError( i18nc( "Error message (sytem-generated description)",
                          "Unexpected response from server (%1)",
                          resultType ),
                  MsnSocketBase::ERROR_SOAP_RESPONSE );
}




/**
 * @brief Internal function to parse the SOAP fault.
 *
 * This method detects the faultcode <code>wsse:FailedAuthentication</code>,
 * to fire a loginIncorrect() signal instead of soapError().
 *
 * It also traps SOAP fauls which are used to redirect the client.
 * So far this has been observed with \@msn.com passport accounts.
 *
 * The SOAP fault for an incorrect login looks like:
 * @code
<?xml version='1.0' encoding='UTF-8'?>
<S:Envelope
  xmlns:S="http://schemas.xmlsoap.org/soap/envelope/"
  xmlns:wsse="http://schemas.xmlsoap.org/ws/2003/06/secext"
  xmlns:psf="http://schemas.microsoft.com/Passport/SoapServices/SOAPFault">
  <S:Header>
    <psf:pp xmlns:psf="http://schemas.microsoft.com/Passport/SoapServices/SOAPFault">
      <psf:serverVersion>1</psf:serverVersion>
      <psf:authstate>0x80048800</psf:authstate>
      <psf:reqstatus>0x80048821</psf:reqstatus>
      <psf:serverInfo Path="Live1" RollingUpgradeState="ExclusiveNew" LocVersion="0"
                      ServerTime="2007-04-22T21:22:38Z">BAYPPLOGN2B32 2006.10.31.02.02.43</psf:serverInfo>
      <psf:cookies/>
      <psf:response/>
    </psf:pp>
  </S:Header>
  <S:Fault>
    <faultcode>wsse:FailedAuthentication</faultcode>
    <faultstring>Authentication Failure</faultstring>
  </S:Fault>
</S:Envelope>
 * @endcode
 *
 * The SOAP fault for a redirect looks like:
 * @code
<?xml version='1.0' encoding='UTF-8'?>
<S:Envelope
  xmlns:S="http://schemas.xmlsoap.org/soap/envelope/"
  xmlns:wsse="http://schemas.xmlsoap.org/ws/2003/06/secext"
  xmlns:psf="http://schemas.microsoft.com/Passport/SoapServices/SOAPFault">
  <S:Header>
    <psf:pp xmlns:psf="http://schemas.microsoft.com/Passport/SoapServices/SOAPFault">
      <psf:serverVersion>1</psf:serverVersion>
      <psf:authstate>0x80048800</psf:authstate>
      <psf:reqstatus>0x80048852</psf:reqstatus>
      <psf:serverInfo Path="Live1" RollingUpgradeState="ExclusiveNew" LocVersion="0"
                      ServerTime="2007-05-05T13:49:58Z">BAYPPLOGN2A01 2007.04.23.14.59.31</psf:serverInfo>
      <psf:cookies/>
      <psf:response/>
    </psf:pp>
  </S:Header>
  <S:Fault>
    <faultcode>psf:Redirect</faultcode>
    <psf:redirectUrl>https://msnia.login.live.com/pp450/RST.srf</psf:redirectUrl>
    <faultstring>Authentication Failure</faultstring>
  </S:Fault>
</S:Envelope>
@endcode
 *
 * Note the <code>Fault</code> object is not in
 * a <code>Body</code> but <code>Envelope</code> element.
 *
 * @param message  The SOAP response message.
 */
void PassportLoginService::parseSoapFault( SoapMessage *message )
{
  // Should never happen
  if( ! message->isFaultMessage() )
  {
    kWarning() << "Incorrect fault detected on incoming message";
    return;
  }

  const QString faultCode( message->getFaultCode() );

  // React accordingly to the received error if related to login issues,
  // if not, fall back to the base class fault parsing
  if( faultCode == "wsse:FailedAuthentication" )
  {
#ifdef KMESSDEBUG_PASSPORTLOGINSERVICE
    kDebug() << "Authentication failed!";
#endif

    emit loginIncorrect();
  }
  // The tickets have expired, require new ones
  else if( faultCode == "q0:BadContextToken" )
  {
    login();

    // Resend the failed message
    SoapMessage *messageCopy = getCurrentRequest( true /* copy */ );
    sendSecureRequest( messageCopy, inProgressRequests_.value( getCurrentRequest() ) );
  }
  else
  {
    // Relay parsing to subclasses
    parseSecureFault( message );
  }

  // Remove the request from the list
  inProgressRequests_.remove( getCurrentRequest() );
}



/**
 * @brief Internal function to extract security tokens from the server response.
 *
 * This handles the <code>wst:RequestSecurityTokenResponseCollection</code> response node.
 *
 * The result messsage looks like:
 * @code
<?xml version="1.0" encoding="utf-8" ?>
<S:Envelope xmlns:S="http://schemas.xmlsoap.org/soap/envelope/">
  <S:Header>
    <psf:pp xmlns:psf="http://schemas.microsoft.com/Passport/SoapServices/SOAPFault">
      <psf:serverVersion>1</psf:serverVersion>
      <psf:PUID>0006000087144D8E</psf:PUID>
      <psf:configVersion>3.100.2199.0</psf:configVersion>
      <psf:uiVersion>3.0.869.0</psf:uiVersion>
      <psf:authstate>0x48803</psf:authstate>
      <psf:reqstatus>0x0</psf:reqstatus>
      <psf:serverInfo Path="Live1" RollingUpgradeState="ExclusiveNew" LocVersion="0" ServerTime="2007-04-29T11:30:36Z">BAYPPLOGN2B29 2007.04.23.14.59.31</psf:serverInfo>
      <psf:cookies/>
      <psf:browserCookies>
        <psf:browserCookie Name="MH" URL="http://www.msn.com">MH=; path=/; domain=.msn.com; expires=Wed, 30-Dec-2037 16:00:00 GMT</psf:browserCookie>
        <psf:browserCookie Name="MHW" URL="http://www.msn.com">MHW=; path=/; domain=.msn.com; expires=Thu, 30-Oct-1980 16:00:00 GMT</psf:browserCookie>
        <psf:browserCookie Name="MH" URL="http://www.live.com">MH=; path=/; domain=.live.com; expires=Wed, 30-Dec-2037 16:00:00 GMT</psf:browserCookie>
        <psf:browserCookie Name="MHW" URL="http://www.live.com">MHW=; path=/; domain=.live.com; expires=Thu, 30-Oct-1980 16:00:00 GMT</psf:browserCookie>
      </psf:browserCookies>
      <psf:credProperties>
        <psf:credProperty Name="MainBrandID"></psf:credProperty>
        <psf:credProperty Name="BrandIDList"></psf:credProperty>
        <psf:credProperty Name="IsWinLiveUser">true</psf:credProperty>
        <psf:credProperty Name="CID">e95c54c3221ebb92</psf:credProperty>
      </psf:credProperties>
      <psf:extProperties></psf:extProperties>
      <psf:response/>
    </psf:pp>
  </S:Header>
  <S:Body>
    <wst:RequestSecurityTokenResponseCollection
      xmlns:S="http://schemas.xmlsoap.org/soap/envelope/"
      xmlns:wst="http://schemas.xmlsoap.org/ws/2004/04/trust"
      xmlns:wsse="http://schemas.xmlsoap.org/ws/2003/06/secext"
      xmlns:wsu="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd"
      xmlns:saml="urn:oasis:names:tc:SAML:1.0:assertion"
      xmlns:wsp="http://schemas.xmlsoap.org/ws/2002/12/policy"
      xmlns:psf="http://schemas.microsoft.com/Passport/SoapServices/SOAPFault">
      <wst:RequestSecurityTokenResponse>
        <wst:TokenType>urn:passport:legacy</wst:TokenType>
        <wsp:AppliesTo xmlns:wsa="http://schemas.xmlsoap.org/ws/2004/03/addressing">
          <wsa:EndpointReference>
            <wsa:Address>http://Passport.NET/tb</wsa:Address>
          </wsa:EndpointReference>
        </wsp:AppliesTo>
        <wst:LifeTime>
          <wsu:Created>2007-04-29T11:30:36Z</wsu:Created>
          <wsu:Expires>2007-04-30T11:30:36Z</wsu:Expires>
        </wst:LifeTime>
        <wst:RequestedSecurityToken>
          <EncryptedData xmlns="http://www.w3.org/2001/04/xmlenc#"
            Id="BinaryDAToken0" Type="http://www.w3.org/2001/04/xmlenc#Element">
            <EncryptionMethod Algorithm="http://www.w3.org/2001/04/xmlenc#tripledes-cbc"></EncryptionMethod>
            <ds:KeyInfo xmlns:ds="http://www.w3.org/2000/09/xmldsig#">
              <ds:KeyName>http://Passport.NET/STS</ds:KeyName>
            </ds:KeyInfo>
            <CipherData>
              <CipherValue>...the token for live authentication...</CipherValue>
            </CipherData>
          </EncryptedData>
        </wst:RequestedSecurityToken>
        <wst:RequestedTokenReference>
          <wsse:KeyIdentifier ValueType="urn:passport"></wsse:KeyIdentifier>
          <wsse:Reference URI="#BinaryDAToken0"></wsse:Reference>
        </wst:RequestedTokenReference>
        <wst:RequestedProofToken>
          <wst:BinarySecret>...other token for live authentication...</wst:BinarySecret>
        </wst:RequestedProofToken>
      </wst:RequestSecurityTokenResponse>
      <wst:RequestSecurityTokenResponse>
        <wst:TokenType>urn:passport:legacy</wst:TokenType>
        <wsp:AppliesTo xmlns:wsa="http://schemas.xmlsoap.org/ws/2004/03/addressing">
          <wsa:EndpointReference>
            <wsa:Address>messenger.msn.com</wsa:Address>
          </wsa:EndpointReference>
        </wsp:AppliesTo>
        <wst:LifeTime>
          <wsu:Created>2007-04-29T11:30:36Z</wsu:Created>
          <wsu:Expires>2007-04-29T11:38:56Z</wsu:Expires>
        </wst:LifeTime>
        <wst:RequestedSecurityToken>
          <wsse:BinarySecurityToken Id="PPToken1">...the token for the USR command...</wsse:BinarySecurityToken>
        </wst:RequestedSecurityToken>
        <wst:RequestedTokenReference>
          <wsse:KeyIdentifier ValueType="urn:passport"></wsse:KeyIdentifier>
          <wsse:Reference URI="#PPToken1"></wsse:Reference>
        </wst:RequestedTokenReference>
      </wst:RequestSecurityTokenResponse>
    </wst:RequestSecurityTokenResponseCollection>
  </S:Body>
</S:Envelope>
@endcode
 *
 * @param message  The SOAP response message.
 */
void PassportLoginService::parseSoapResult( SoapMessage *message )
{
  const QDomElement body( message->getBody().firstChildElement() );

  // Remove the request from the list
  inProgressRequests_.remove( getCurrentRequest() );

  // Pass all other messages to child classes
  if( body.nodeName() != "wst:RequestSecurityTokenResponseCollection" )
  {
    parseSecureResult( message );
    return;
  }

#ifdef KMESSDEBUG_PASSPORTLOGINSERVICE
  kDebug() << "Current date:" << QDateTime::currentDateTime();
  kDebug() << "Obtaining security tokens..";
#endif

  const QDomNodeList authTokens( body.childNodes() );
  if( authTokens.count() == 0 )
  {
    // Likely a login error.
    emit loginIncorrect();
    return;
  }

  const QDateTime &now = QDateTime::currentDateTime();
#ifdef KMESSDEBUG_PASSPORTLOGINSERVICE
  kDebug() << "Current date and time:" << now;
#endif

  // Set the token and proof for hotmail live services (mail, spaces etc..)
  QHash<QString,QString> tokens;
  tokenExpirationDates_.clear();
  for( uint index = 0; index < authTokens.length(); ++index )
  {
    const QDomNode tokenResponse( authTokens.item( index ) );

    const QString referenceEndPoint( XmlFunctions::getNodeValue( tokenResponse, "AppliesTo/EndpointReference/Address" ) );

    // Extract the token's expiration date
    const QString creationString  ( XmlFunctions::getNodeValue( tokenResponse, "LifeTime/Created" ) );
    const QString expirationString( XmlFunctions::getNodeValue( tokenResponse, "LifeTime/Expires" ) );

    const QDateTime &creationDate   = QDateTime::fromString( creationString,   Qt::ISODate );
    const QDateTime &expirationDate = QDateTime::fromString( expirationString, Qt::ISODate );

    int ticketDuration;

    /**
     * Compute the duration in seconds of the token, and apply it to the current local time.
     * This allows us to avoid meddling with timezones. Which sucks, specially in the cases where:
     * 1) The system is not set to the correct timezone;
     * 2) KDE lacks timezone packages;
     * 3) Bugs in KDE (maybe..);
     * Also consider that QDateTime discards the timezone in ISO dates, and you will have an idea
     * why we're doing this.
     *
     * Also this process doesn't take into account any time passed from the ticket generation to the
     * computation: if a ticket which expires in 100 seconds is generated at 16:30:20 but we receive
     * it at 16:30:24, our computed duration will 100 seconds, but it should be 96 seconds.
     * However we take this possible delay into account elsewhere: when we need to use a token, we
     * check the token validity with 10 seconds margin (ie if we need a token which expires in 9s,
     * it's expired already for us.
     */
    if( ! creationDate.isValid() || ! expirationDate.isValid() )
    {
#ifdef KMESSDEBUG_PASSPORTLOGINSERVICE
      kDebug() << "Invalid token creation/expiration dates. Defaulting to 8 hours duration for token on next line:";
#endif
      ticketDuration = 3600 * 8;
    }
    else
    {
      ticketDuration = creationDate.secsTo( expirationDate );
    }

    const QDateTime &localExpirationDate = now.addSecs( ticketDuration );

#ifdef KMESSDEBUG_PASSPORTLOGINSERVICE
    kDebug() << "--";
    kDebug() << "Found token for endpoint:" << referenceEndPoint << "with a duration of" << ticketDuration << "seconds.";
    kDebug() << "Created:" << creationString
             << "Expires:" << expirationString;
    kDebug() << "Expires at local time:" << localExpirationDate;
#endif

    if( referenceEndPoint == "http://Passport.NET/tb" )
    {
      tokens.insert( "Passport",
                     XmlFunctions::getNodeValue( tokenResponse,
                                                 "RequestedSecurityToken/EncryptedData/CipherData/CipherValue" ) );
      tokens.insert( "PassportProof",
                     XmlFunctions::getNodeValue( tokenResponse,
                                                 "RequestedProofToken/BinarySecret" ) );
      tokenExpirationDates_.insert( "Passport", localExpirationDate );
    }
    else if( referenceEndPoint == "messengerclear.live.com" )
    {
      tokens.insert( "MessengerClear",
                     XmlFunctions::getNodeValue( tokenResponse,
                                                 "RequestedSecurityToken/BinarySecurityToken" ) );
      tokens.insert( "MessengerClearProof",
                     XmlFunctions::getNodeValue( tokenResponse,
                                                 "RequestedProofToken/BinarySecret" ) );
      tokenExpirationDates_.insert( "MessengerClear", localExpirationDate );
    }
    else if( referenceEndPoint == "messenger.msn.com" )
    {
      tokens.insert( "Messenger",
                     XmlFunctions::getNodeValue( tokenResponse,
                                                 "RequestedSecurityToken/BinarySecurityToken" ) );
      tokenExpirationDates_.insert( "Messenger", localExpirationDate );
    }
    else if( referenceEndPoint == "contacts.msn.com" )
    {
      tokens.insert( "Contacts",
                     XmlFunctions::getNodeValue( tokenResponse,
                                                 "RequestedSecurityToken/BinarySecurityToken" ) );
      tokenExpirationDates_.insert( "Contacts", localExpirationDate );
    }
    else if( referenceEndPoint == "messengersecure.live.com" )
    {
      tokens.insert( "MessengerSecure",
                     XmlFunctions::getNodeValue( tokenResponse,
                                                 "RequestedSecurityToken/BinarySecurityToken" ) );
      tokenExpirationDates_.insert( "MessengerSecure", localExpirationDate );
    }
    else if( referenceEndPoint == "storage.msn.com" )
    {
      tokens.insert( "Storage",
                     XmlFunctions::getNodeValue( tokenResponse,
                                                 "RequestedSecurityToken/BinarySecurityToken" ) );
      tokenExpirationDates_.insert( "Storage", localExpirationDate );
    }
  }

#ifdef KMESSDEBUG_PASSPORTLOGINSERVICE
  kDebug() << "Obtained" << tokens.count() << "security tokens:" << tokens.keys();
#endif

  // Save the tokens
  currentAccount_->setTokens( tokens );

  isWaitingForNewTokens_ = false;

  // Send any message which was waiting for a new token
  QHashIterator<SoapMessage*,QString> it( queuedRequests_ );
  while( it.hasNext() )
  {
    it.next();
    sendSecureRequest( it.key(), it.value() );
  }
#ifdef KMESSDEBUG_PASSPORTLOGINSERVICE
  kDebug() << "Sent" << queuedRequests_.count() << "queued messages.";
#endif


  emit loginSucceeded();
}



/**
 * @brief SOAP call to request the login tokens.
 *
 * This method is called by login().
 * It sends a huge binary blob which looks like this:
 * @code
<?xml version="1.0" encoding="utf-8"?>
<soap:Envelope
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
  <soap:Header>
    <ps:AuthInfo xmlns:ps="http://schemas.microsoft.com/Passport/SoapServices/PPCRL" Id="PPAuthInfo">
      <ps:HostingApp>{7108E71A-9926-4FCB-BCC9-9A9D3F32E423}</ps:HostingApp>
      <ps:BinaryVersion>4</ps:BinaryVersion>
      <ps:UIVersion>1</ps:UIVersion>
      <ps:Cookies></ps:Cookies>
      <ps:RequestParams>AQAAAAIAAABsYwQAAAAzMDg0</ps:RequestParams>
    </ps:AuthInfo>
    <wsse:Security xmlns:wsse="http://schemas.xmlsoap.org/ws/2003/06/secext">
      <wsse:UsernameToken Id="user">
        <wsse:Username>...the login() username...</wsse:Username>
        <wsse:Password>...the login() password...</wsse:Password>
      </wsse:UsernameToken>
    </wsse:Security>
  </soap:Header>
  <soap:Body>
    <ps:RequestMultipleSecurityTokens
      xmlns:ps="http://schemas.microsoft.com/Passport/SoapServices/PPCRL"
      xmlns:wst="http://schemas.xmlsoap.org/ws/2004/04/trust"
      xmlns:wsp="http://schemas.xmlsoap.org/ws/2002/12/policy"
      xmlns:wsa="http://schemas.xmlsoap.org/ws/2004/03/addressing"
      xmlns:wsse="http://schemas.xmlsoap.org/ws/2003/06/secext"
      Id="RSTS">
      <wst:RequestSecurityToken Id="RST0">
        <wst:RequestType>http://schemas.xmlsoap.org/ws/2004/04/security/trust/Issue</wst:RequestType>
        <wsp:AppliesTo>
          <wsa:EndpointReference>
            <wsa:Address>http://Passport.NET/tb</wsa:Address>
          </wsa:EndpointReference>
        </wsp:AppliesTo>
      </wst:RequestSecurityToken>
      <wst:RequestSecurityToken Id="RST1">
        <wst:RequestType>http://schemas.xmlsoap.org/ws/2004/04/security/trust/Issue</wst:RequestType>
        <wsp:AppliesTo>
          <wsa:EndpointReference>
            <wsa:Address>messenger.msn.com</wsa:Address>
          </wsa:EndpointReference>
        </wsp:AppliesTo>
        <wsse:PolicyReference URI="...the login() parameters..."></wsse:PolicyReference>
      </wst:RequestSecurityToken>
    </ps:RequestMultipleSecurityTokens>
  </soap:Body>
</soap:Envelope>
@endcode
 */
void PassportLoginService::requestMultipleSecurityTokens()
{
#ifdef KMESSDEBUG_PASSPORTLOGINSERVICE
  kDebug() << "Requesting security tokens";
#endif

  const QString authParams( KMessShared::htmlEscape( QUrl::fromPercentEncoding( authenticationParameters_.toUtf8() )
                                                     .replace( ",", "&" ) ) );

  QString header( "<ps:AuthInfo"
                  " xmlns:ps=\"http://schemas.microsoft.com/Passport/SoapServices/PPCRL\""
                  " Id=\"PPAuthInfo\">\n"
                  "  <ps:HostingApp>{7108E71A-9926-4FCB-BCC9-9A9D3F32E423}</ps:HostingApp>\n"
                  "  <ps:BinaryVersion>4</ps:BinaryVersion>\n"
                  "  <ps:UIVersion>1</ps:UIVersion>\n"
                  "  <ps:Cookies></ps:Cookies>\n"
                  "<ps:RequestParams>AQAAAAIAAABsYwQAAAAzMDg0</ps:RequestParams>\n"
                  "</ps:AuthInfo>\n"
                  "<wsse:Security"
                  " xmlns:wsse=\"http://schemas.xmlsoap.org/ws/2003/06/secext\">\n"
                  "  <wsse:UsernameToken Id=\"user\">\n"
                  "    <wsse:Username>" + KMessShared::htmlEscape( handle_   ) + "</wsse:Username>\n"
                  "    <wsse:Password>" + KMessShared::htmlEscape( password_ ) + "</wsse:Password>\n"
                  "  </wsse:UsernameToken>\n"
                  "</wsse:Security>" );

  QString body( "<ps:RequestMultipleSecurityTokens"
                " xmlns:ps=\"http://schemas.microsoft.com/Passport/SoapServices/PPCRL\""
                " xmlns:wst=\"http://schemas.xmlsoap.org/ws/2004/04/trust\""
                " xmlns:wsp=\"http://schemas.xmlsoap.org/ws/2002/12/policy\""
                " xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/03/addressing\""
                " xmlns:wsse=\"http://schemas.xmlsoap.org/ws/2003/06/secext\""
                " Id=\"RSTS\">\n"

                // Unknown, but it's required for the request to succeed
                "  <wst:RequestSecurityToken Id=\"RST0\">\n"
                "    <wst:RequestType>http://schemas.xmlsoap.org/ws/2004/04/security/trust/Issue</wst:RequestType>\n"
                "    <wsp:AppliesTo>\n"
                "      <wsa:EndpointReference>\n"
                "        <wsa:Address>http://Passport.NET/tb</wsa:Address>\n"
                "      </wsa:EndpointReference>\n"
                "    </wsp:AppliesTo>\n"
                "  </wst:RequestSecurityToken>\n"

                // Authentication for messenger
                "  <wst:RequestSecurityToken Id=\"RST1\">\n"
                "    <wst:RequestType>http://schemas.xmlsoap.org/ws/2004/04/security/trust/Issue</wst:RequestType>\n"
                "    <wsp:AppliesTo>\n"
                "      <wsa:EndpointReference>\n"
                "        <wsa:Address>messengerclear.live.com</wsa:Address>\n"
                "      </wsa:EndpointReference>\n"
                "    </wsp:AppliesTo>\n"
                "    <wsse:PolicyReference URI=\"" + authParams + "\"></wsse:PolicyReference>\n"
                "  </wst:RequestSecurityToken>\n"

                // Messenger website authentication
                "  <wst:RequestSecurityToken Id=\"RST2\">\n"

                "    <wst:RequestType>http://schemas.xmlsoap.org/ws/2004/04/security/trust/Issue</wst:RequestType>\n"
                "    <wsp:AppliesTo>\n"
                "      <wsa:EndpointReference>\n"
                "        <wsa:Address>messenger.msn.com</wsa:Address>\n"
                "      </wsa:EndpointReference>\n"
                "    </wsp:AppliesTo>\n"

                "    <wsse:PolicyReference URI=\"?id=507\"></wsse:PolicyReference>\n"

                "  </wst:RequestSecurityToken>\n"

                // Authentication for the Contact server
                "  <wst:RequestSecurityToken Id=\"RST3\">\n"
                "    <wst:RequestType>http://schemas.xmlsoap.org/ws/2004/04/security/trust/Issue</wst:RequestType>\n"
                "    <wsp:AppliesTo>\n"
                "      <wsa:EndpointReference>\n"
                "        <wsa:Address>contacts.msn.com</wsa:Address>\n"
                "      </wsa:EndpointReference>\n"
                "    </wsp:AppliesTo>\n"
                "    <wsse:PolicyReference URI=\"MBI\"></wsse:PolicyReference>\n"
                "  </wst:RequestSecurityToken>\n"

                // Authentication for the messenger live ( example: to send offline messages )
                "  <wst:RequestSecurityToken Id=\"RST4\">\n"
                "    <wst:RequestType>http://schemas.xmlsoap.org/ws/2004/04/security/trust/Issue</wst:RequestType>\n"
                "    <wsp:AppliesTo>\n"
                "      <wsa:EndpointReference>\n"
                "        <wsa:Address>messengersecure.live.com</wsa:Address>\n"
                "      </wsa:EndpointReference>\n"
                "    </wsp:AppliesTo>\n"
                "    <wsse:PolicyReference URI=\"MBI_SSL\"></wsse:PolicyReference>\n"
                "  </wst:RequestSecurityToken>\n"

                // Authentication for the storage msn site ( for roaming service )
                "  <wst:RequestSecurityToken Id=\"RST5\">\n"
                "  <wst:RequestType>http://schemas.xmlsoap.org/ws/2004/04/security/trust/Issue</wst:RequestType>\n"
                "    <wsp:AppliesTo>\n"
                "      <wsa:EndpointReference>\n"
                "        <wsa:Address>storage.msn.com</wsa:Address>\n"
                "      </wsa:EndpointReference>\n"
                "    </wsp:AppliesTo>\n"
                "    <wsse:PolicyReference URI=\"MBI\"></wsse:PolicyReference>\n"
                "  </wst:RequestSecurityToken>\n"

                "</ps:RequestMultipleSecurityTokens>" );

  isWaitingForNewTokens_ = true;

  sendRequest( new SoapMessage( SERVICE_URL_RST_SERVICE,
                                QString(), // This service doesn't require an action
                                header,
                                body ) );
}



/**
 * @brief Send the authenticated SOAP request from a subclass
 * @internal
 *
 * This method sends authenticated SOAP messages. It first needs to verify
 * if the session's ticket tokens are still valid - and if not, request new
 * ones then sends the request.
 *
 * @param message  The SOAP response message.
 */
void PassportLoginService::sendSecureRequest( SoapMessage *message, const QString &requiredTokenName )
{
  // Send requests without authentication
  if( requiredTokenName.isEmpty() )
  {
    sendRequest( message );
    return;
  }

#ifdef KMESSDEBUG_PASSPORTLOGINSERVICE
  kDebug() << "Sending secure request with token:" << requiredTokenName;
#endif

  const QString tokenValue( currentAccount_->getToken( requiredTokenName ) );
  const QDateTime &tokenExpiration = tokenExpirationDates_[ requiredTokenName ];

  int timeDifference = QDateTime::currentDateTime().secsTo( tokenExpiration );

#ifdef KMESSDEBUG_PASSPORTLOGINSERVICE
  kDebug() << "Token has a value?" << (!tokenValue.isEmpty());
  kDebug() << "Current time:" << QDateTime::currentDateTime();
  kDebug() << "Token expires on:" << tokenExpiration << ", that is within seconds:" << timeDifference;
#endif

  // If there is no value for this token, or the token is about to expire, ask a new one
  if( tokenValue.isEmpty() || timeDifference < 10 )
  {
#ifdef KMESSDEBUG_PASSPORTLOGINSERVICE
    kDebug() << "Token is expired or invalid.";
#endif

    // Queue the message
    if( ! queuedRequests_.contains( message ) )
    {
      queuedRequests_.insert( message, requiredTokenName );
    }

    // We're already waiting for new tokens
    if( isWaitingForNewTokens_ )
    {
#ifdef KMESSDEBUG_PASSPORTLOGINSERVICE
      kDebug() << "Already requesting new tokens.";
#endif
      return;
    }

#ifdef KMESSDEBUG_PASSPORTLOGINSERVICE
    kDebug() << "Asking for new tokens.";
#endif

    // Ask for new tokens, altogether instead of just one at a time,
    // for convenience and bandwidth saving
    login();
    return;
  }

#ifdef KMESSDEBUG_PASSPORTLOGINSERVICE
  kDebug() << "Token is valid, adding it to the request.";
#endif

  // Insert the token value within the SOAP message header
  QDomNodeList list;
  QDomElement header( message->getHeader().toElement() );

  list = header.elementsByTagName( "TicketToken" );

  // There is no <TicketToken> tag: search for the <Ticket> tag
  if( list.isEmpty() )
  {
    list = header.elementsByTagName( "Ticket" );
  }

  // None of the ticket token tags were found, send the message as is
  if( list.isEmpty() )
  {
    kWarning() << "Token tag not found! Sending the request as is.";

    sendRequest( message );
    return;
  }

  for( uint index = 0; index < list.length(); ++index )
  {
    QDomElement item( list.item( index ).toElement() );

    // <TicketToken> tags directly contain the token value
    if( item.tagName() == "TicketToken" )
    {
#ifdef KMESSDEBUG_PASSPORTLOGINSERVICE
      kDebug() << "TicketToken tag found!";
#endif

      // If there is a child text node, simply replace its text
      if( item.firstChild().nodeType() == QDomNode::TextNode )
      {
        item.firstChild().toText().setNodeValue( tokenValue );
      }
      else
      {
        QDomText text( item.ownerDocument().createTextNode( tokenValue ) );
        item.appendChild( text );
      }
      break;
    }

    // <Ticket> tags contain the token value within the "passport" attribute
    if( item.tagName() == "Ticket" )
    {
#ifdef KMESSDEBUG_PASSPORTLOGINSERVICE
      kDebug() << "Ticket tag found!";
#endif
      item.setAttribute( "passport", tokenValue );
      break;
    }
  }

  // Remove the message from the queue (if it was there) and add it to the in progress messages
  if( queuedRequests_.contains( message ) )
  {
    queuedRequests_.remove( message );
  }
  inProgressRequests_.insert( message, requiredTokenName );

#ifdef KMESSDEBUG_PASSPORTLOGINSERVICE
  kDebug() << "Sending authenticated request.";
#endif

  // Send the changed message
  sendRequest( message );
}



#include "passportloginservice.moc"
