/***************************************************************************
                          roamingservice.h -  description
                             -------------------
    begin                : sun Jan 4 2009
    copyright            : (C) 2009 by Antonio Nastasi
    email                : sifcenter(at)gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ROAMINGSERVICE_H
#define ROAMINGSERVICE_H

#include "passportloginservice.h"


// Forward declarations
class QNetworkReply;



/**
 * @brief Soap actions for retrieve the address book and membership lists.
 *
 *
 * @author Antonio Nastasi
 * @ingroup NetworkSoap
 */
class RoamingService : public PassportLoginService
{
  Q_OBJECT

  public:
    // Constructor
                    RoamingService( QObject *parent );
    // Destructor
                   ~RoamingService();
    // Get the profile for given contact
    void            getProfile( const QString& cid );
    // Create a profile
    void            createProfile();
    // update the profile of the current account
    void            updateProfile();
    // update the display picture of the current acccount
    void            updateDisplayPicture();

  private slots:
    // Received the display picture from the server
    void            receivedDisplayPicture( QNetworkReply *reply );

  private: // Private methods
    // Create the common header for this service
    QString         createCommonHeader() const;
    // Parse the SOAP fault
    void            parseSecureFault( SoapMessage *message );
    // The connection received the full response
    void            parseSecureResult( SoapMessage *message );
    // Process the getProfile response
    void            processProfileResult( const QDomElement &body );
    // create a document on the server
    void            createDocument( const QString& name, const QString& mimeType, const QString& data );
    // find documents on the server
    void            findDocuments();
    // create relationships on the server
    void            createRelationships( const QString& source_rid, const QString& target_rid );
    // delete relationships on the server
    void            deleteRelationships( const QString& source_rid, const QString& target_rid );

  private: // Private members -- why were these static? for what purpose?
    QString  cid_;
    QString  profileResourceId_;
    QString  displayPictureResourceId_;
    QString  lastKnownPersonalMessage_;
    QString  lastKnownDisplayPicturePath_;
    QString  lastKnownFriendlyName_;

  signals:
    // A friendly name was received
    void receivedFriendlyName( const QString& newFriendlyName );
    // A display picture was received
    void receivedDisplayPicture( const QString& pictureUrl );
    // A personal message was received
    void receivedPersonalMessage( const QString& personalMessage );
};

#endif
