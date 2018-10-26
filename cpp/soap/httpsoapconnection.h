/***************************************************************************
                          httpsoapconnection.h -  description
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

#ifndef HTTPSOAPCONNECTION_H
#define HTTPSOAPCONNECTION_H

#include "../msnsocketbase.h"

#include <QDomElement>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QTimer>
#include <QUrl>


class QAuthenticator;
class QNetworkAccessManager;
class QNetworkReply;
class QSslError;

class SoapMessage;
class MimeMessage;



/**
 * @brief SOAP transport over a HTTP connection.
 *
 * The SOAP protocol is used to implement webservices.
 * It's commonly used by .Net applications,
 * MSN Messenger is not an exception here.
 * SOAP is used for new features like:
 * - games / activities.
 * - offline-im.
 * - passport 3.0 authentication.
 * - addressbook data (as of Windows Live Messenger).
 *
 * The class implements SOAP at a basic level.
 * - it operates much like the <code>XmlHttpRequest</code> class works in web browsers.
 * - sniffed XML strings can be sent directly with sendRequest().
 * - no need to translate XML to SOAP encodings.
 * - no need to reverse engineer a WSDL to send SOAP messages.
 * - http://wanderingbarque.com/nonintersecting/2006/11/15/the-s-stands-for-simple/
 *
 * To use this class, extend it to implement method wrappers for the SOAP calls.
 * A request can be sent with sendRequest(). The response is received as SoapMessage in parseSoapResult():
 * overwrite parseSoapResult() to handle the normal responses, and parseSoapFault() for the error responses.
 *
 * @author Diederik van der Boor
 * @author Valerio Pilo
 * @ingroup NetworkSoap
 */
class HttpSoapConnection : public QObject
{
  Q_OBJECT

  public:  // public methods
    // The constructor
    explicit             HttpSoapConnection( QObject *parent = 0 );
    // The destructor
    virtual             ~HttpSoapConnection();

    // Abort all queued requests
    void                 abort();
    // Whether the connection is idle, not processing a SOAP request/response
    bool                 isIdle();

  protected:
    // Return the current request message, if any
    SoapMessage         *getCurrentRequest( bool copy = false ) const;
    // Parse the SOAP fault
    virtual void         parseSoapFault( SoapMessage *message );
    // The connection received the full response
    virtual void         parseSoapResult( SoapMessage *message ) = 0;
    // Send a SOAP request to the webservice
    virtual void         sendRequest( SoapMessage *message, bool urgent = false );
    // Decode UTF-8 text from a SOAP node (usually friendly names).
    QString              textNodeDecode( const QString &string );

  private slots:
    // Send the next request in queue to the endpoint.
    void                 sendNextRequest();
    // Called when the remote server requires authentication
    void                 slotAuthenticationRequired( QNetworkReply *reply, QAuthenticator *authenticator );
    // Called when the request to the remote server finished
    void                 slotRequestFinished( QNetworkReply *reply );
    // Called when a timeout occurred while sending a request
    void                 slotRequestTimeout();
    // Called when an ssl error occurred while setting up the connection
    void                 slotSslErrors( QNetworkReply *reply, const QList<QSslError> &sslErrors );


  private:  // private attributes
    /// The current request which is being processed
    SoapMessage         *currentRequest_;
    /// The list of redirections
    QHash<QString,QString> redirections_;
    /// The redirection counter for each redirection
    QHash<QString,int> redirectionCounts_;
    /// The queue of active requests
    QList<SoapMessage*>  requests_;
    /// The connection manager
    QNetworkAccessManager *http_;
    /// Whether a message is being sent or not
    bool                 isSending_;
	QMutex               lockMutex_;
    /// Timer used to detect timeouts when sending requests
    QTimer               responseTimer_;
    /// The last SOAP action
    QString              soapAction_;

  signals:
    /**
     * @brief Fired when a fatal error occured.
     *
     * This signal is *not* meant to be used when the server answers with an error
     * message, but when the request could not be sent, or when the answer does not
     * arrive within a reasonable timeframe.
     *
     * @param  error  The message to display in the user interface.
     * @param  type   The type of error.
     */
    void                 soapError( QString error, MsnSocketBase::ErrorType type );
    /**
     * @brief Fired when the user needs to be notified about a problem.
     *
     * This is used whenever the SOAP classes have to send messages back to the user.
     *
     * @param  warning      The message to display in the user interface.
     * @param  isImportant  Whether the message is important enough to interrupt the user or not
     */
     void                soapWarning( const QString &warning, bool isImportant );
};

#endif
