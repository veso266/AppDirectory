/***************************************************************************
                          soapmessage.h  -  description
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

#ifndef SOAPMESSAGE_H
#define SOAPMESSAGE_H

#include <QDomNode>
#include <QString>
#include <QUrl>
#include <QVariant>


/**
 * @brief A simple container class for SOAP message data
 *
 * This class is used to hold extra information about the message.
 */
class MessageData
{
  public:
    // Empty constructor
             MessageData() {};
    // Copy constructor
             MessageData( const MessageData &other ) { type = other.type; value = other.value; };
    // Destructor
            ~MessageData() {};

  public: // Public properties
    // Type of message
    QString  type;
    // Data contained within the message
    QVariant value;
};



/**
 * @brief A class to parse SOAP messages.
 *
 * These messages are sent to and received from a remote server by using the HttpSoapConnection class.
 * You can also attach some data to a message, so that data sent along with the request will be available when
 * the response is received.
 *
 * @author Diederik van der Boor
 * @author Valerio Pilo
 * @ingroup NetworkSoap
 */
class SoapMessage
{
  public:  // public methods
    // The constructor
    explicit             SoapMessage( const QString &endPointUrl, const QString &action, const QString &header = QString(), const QString &body = QString(), const MessageData &data = MessageData() );
    // The copy constructor
    explicit             SoapMessage( const SoapMessage &other );
    // The destructor
    virtual             ~SoapMessage();

    // Return the action type
    const QString&       getAction() const;
    // Return the message body's xml tree
    const QDomNode&      getBody() const;
    // Return the associated request data
    const MessageData&   getData() const;
    // Return the URL of the SOAP endpoint to which the message will be sent
    const QString&       getEndPoint() const;
    // Return the message fault's xml tree
    const QDomNode&      getFault() const;
    // Return the error code
    const QString&       getFaultCode() const;
    // Return the error description
    const QString&       getFaultDescription() const;
    // Return the message header's xml tree
    QDomNode&            getHeader();
    // Return the entire message as string
    QByteArray           getMessage() const;
    // Return whether this is an error or a valid response
    bool                 isFaultMessage() const;
    // Return whether this message contains valid useable data
    bool                 isValid() const;
    // Change the associated request data
    void                 setData( const MessageData &data );
    // Parse an incoming message
    void                 setMessage( const QString &message );


  private:  // Protected properties
    // The message type
    QString              action_;
    // The message contents
    QDomNode             body_;
    // Data associated to the message
    MessageData          data_;
    // The target endpoint URL
    QString              endPoint_;
    // Contents of the message fault notice
    QDomNode             fault_;
    // The error code from the soap response
    QString              faultCode_;
    // The error description from the soap response
    QString              faultDescription_;
    // Flag to easily check if the message contains useable data
    bool                 isValid_;
    // The content header
    QDomNode             header_;
};

#endif
