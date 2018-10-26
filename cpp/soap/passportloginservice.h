/***************************************************************************
                          passportloginservice.h  -  description
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

#ifndef SSLLOGINHANDLER_H
#define SSLLOGINHANDLER_H

#include "httpsoapconnection.h"
#include "../../kmessdebug.h"

#include <QDateTime>


// Forward declarations
class CurrentAccount;
class SoapMessage;



/**
 * @brief SOAP calls to the passport authentication service.
 *
 * This class can be used to retrieve the authentication tokens
 * from the Passport web service.
 * Tokens can be used to access other specific services.
 *
 * It can be used in two ways:
 * - Instance it, connect to the login*() signals and call
 *   login(). This can be used during the initial authentication,
 *   to retrieve the tokens for the "USR" command.
 * - Subclass it and use the parseSecure*() and
 *   sendSecureRequest() methods to send and retrieve your own
 *   SOAP messages.
 *
 * The first case is trivial and doesn't need much explanation.
 * The second is more complex: many Passport web services need
 * authentication tokens to be present in the request header.
 * These tokens expire after a certain amount of time, just like
 * browser cookies.
 *
 * This class allows for transparent token management. From the
 * subclasses, you can simply leave the header
 * @code<TicketToken>@endcode or @code<Ticket>@endcode
 * tags empty: this class will always fill in the correct fields
 * with a valid non-expired token, and will automatically send
 * token update requests when they expire.
 *
 *
 * This replaces the SSL-based Passport 1.4 login.
 * The old class used a simple HTTP header to send results,
 * this class has to send an large XML blob instead.
 * The webservice is actually called "RST", which likely stands
 * for "Request Security Token" considering the XML contents.
 *
 * For additional documentation about Passport 3.0, see
 * http://msnpiki.msnfanatic.com/index.php/MSNP13:SOAPTweener
 *
 * @author Diederik van der Boor
 * @author Valerio Pilo
 * @ingroup NetworkSoap
 */
class PassportLoginService : public HttpSoapConnection
{
  Q_OBJECT

  public:
    // The constructor
                         PassportLoginService( QObject *parent = 0 );
    // The destructor
    virtual             ~PassportLoginService();
    // Start the login process
    void                 login( const QString &parameters = QString() );
    // Compute the string for live hotmail access
    static const QString createHotmailToken( const QString &passportToken, const QString &proofToken,
                                             const QString &folder );

  protected: // Protected members
    // Bounce the authenticated SOAP fault to a subclass
    virtual void         parseSecureFault( SoapMessage *message );
    // Bounce the authenticated SOAP response to a subclass
    virtual void         parseSecureResult( SoapMessage *message );
    // Send the authenticated SOAP request from a subclass
    void                 sendSecureRequest( SoapMessage *message, const QString &requiredTokenName = QString() );

  private:
    // Parse the SOAP fault
    void                 parseSoapFault( SoapMessage *message );
    // Process server responses
    void                 parseSoapResult( SoapMessage *message );
    // Send the authentication request
    void                 requestMultipleSecurityTokens();

  protected: // Protected attributes
    // Current account instance
    CurrentAccount      *currentAccount_;

  private: // Private attributes
    /// The list of parameters sent by the notification server
    QString              authenticationParameters_;
    /// The user's handle
    QString              handle_;
    /// The user's password
    QString              password_;
    /// Whether we're waiting for new authentication tokens
    bool                 isWaitingForNewTokens_;
    /// The queue of requests waiting new authentication tokens
    QHash<SoapMessage*,QString>  queuedRequests_;
    /// The list of requests which are being sent
    QHash<SoapMessage*,QString>  inProgressRequests_;

  private: // Private static attributes
    /// The expiration dates of the security tokens
    static QHash<QString,QDateTime>  tokenExpirationDates_;

  signals : // Public signals
    /**
     * @brief Fired when the login failed because a wrong username/password was used.
     */
    void                 loginIncorrect();

    /**
     * @brief Fired when the login succeeded. (version for MSN Protocol 15)
     */
    void                 loginSucceeded();
};

#endif
