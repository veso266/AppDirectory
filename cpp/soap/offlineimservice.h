/***************************************************************************
                          offlineimservice.h -  description
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

#ifndef OFFLINEIMSERVICE_H
#define OFFLINEIMSERVICE_H

#include "passportloginservice.h"

#include <QHash>


class ChatMessage;
class QDateTime;



/**
 * @brief SOAP calls to the Offline-IM webservice.
 *
 * This class is used by the MsnNotificationConnection to retrieve Offline-IM messages.
 * The following methods of the webservice methods are available:
 * - deleteMessages()
 * - getMessage()
 * - getMetaData()
 *
 * Internally, a connection is made to the Offline-IM webservice.
 * The results are processed internally too, so the messageReceived()
 * and metaDataReceived() signals return the parsed results already.
 *
 * For additional documentation about Offline-IM,
 * see: http://msnpiki.msnfanatic.com/index.php/MSNP13:Offline_IM
 *
 * @author Diederik van der Boor
 * @ingroup NetworkSoap
 */
class OfflineImService : public PassportLoginService
{
  Q_OBJECT

  public:  // public methods
    // The receiving mode constructor
                         OfflineImService( const QString &authT, const QString &authP, QObject *parent = 0 );
    // The sending mode constructor
                         OfflineImService( QObject *parent );
    // The destructor
    virtual             ~OfflineImService();

    // Delete messages from the storage space
    void                 deleteMessages( const QStringList &messageIds );
    // Request an offline message.
    void                 getMessage( const QString &messageId, bool markAsRead = false );
    // Request the Mail-Data field over SOAP.
    void                 getMetaData();
    // Send an offline message
    void                 sendMessage( const QString &to, const QString &message );

  private:
    // Extract the email address from an RFC822 formatted string.
    QString              extractRFC822Address( const QString &address );
    // Process the SOAP fault returned when sending an offline message.
    void                 parseSecureFault( SoapMessage *message );
    // Process server responses
    void                 parseSecureResult( SoapMessage *message );
    // Process the getMessage response
    void                 processGetMessageResult( SoapMessage *message );
    // Internal method to store a message in the offline-im storage.
    void                 storeMessage( const QString &to, const QString &message, int sequenceNum );

  private:
    /// The <code>t</code> value of the passport cookie.
    QString              authT_;
    /// The <code>p</code> value of the passport cookie.
    QString              authP_;
    // Offline message sequence number
    int                  nextSequenceNum_;
    /// The passport header to send with SOAP
    QString              passportCookieHeader_;
    // The passport service for require the new ticket
    PassportLoginService *passportService_;
    // GuID for the OIM session
    QString              runID_;


  signals:
    /**
     * @brief Fired when the response of the getMetaData() call arrived.
     *
     * It returns the same XML element which are normally
     * received at the notification connection.
     *
     * @param  metaData  The received data.
     */
    void                 metaDataReceived( QDomElement metaData );

    /**
     * @brief Fired when the response of the getMessage() call arrived.
     *
     * When multiple messages are downloaded,
     * use the date value to order the messages.
     * The runId and sequenceNum fields may appear in a different order.
     *
     * @param  messageId    Identifier of the offline message.
     * @param  from         The contact who sent the message.
     * @param  to           The recipient of the message; the current user.
     * @param  date         The original date of the message.
     * @param  body         The message body; the message typed by the sender.
     * @param  runId        Identifier of the chat session.
     * @param  sequenceNum  Identifier of the message in the chat session.
     */
    void                 messageReceived( const QString &messageId, const QString &from, const QString &to,
                                          const QDateTime &date, const QString &body,
                                          const QString &runId, int sequenceNum );

    void                 sendMessageFailed( const QString &to, const MimeMessage &message );
};

#endif
