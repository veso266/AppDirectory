/***************************************************************************
                          msnappdirectoryservice.h -  description
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

#ifndef MSNAPPDIRECTORYSERVICE_H
#define MSNAPPDIRECTORYSERVICE_H

#include "httpsoapconnection.h"

#include <QList>
#include <QObject>


class QDomElement;



/**
 * Wrapper for SOAP calls to the MSN application directory service.
 *
 * @author Diederik van der Boor
 * @ingroup NetworkSoap
 */
class MsnAppDirectoryService : public HttpSoapConnection
{
  Q_OBJECT

  public:
    // Types of service listings to request
    enum MsnAppDirectoryServiceType { GAMES, ACTIVITIES };

    struct Entry
    {
      int     entryId;

      QString subscriptionUrl;
      QString error;
      QString locale;
      QString sequence;
      QString name;
      QString description;
      QString url;
      QString iconUrl;
      QString appIconUrl;
      QString type;
      QString location;
      QString clientVersion;
      int     page;
      int     categoryId;
      int     passportSiteId;
      int     height;
      int     width;
      int     minUsers;
      int     maxUsers;
      int     maxPacketRate;
      int     appType;
      bool    kids;
      bool    enableIp;
      bool    activeX;
      bool    sendFile;
      bool    receiveIM;
      bool    replaceIM;
      bool    windows;
      bool    userProperties;
      bool    hidden;
    };

  public:  // public methods
    // The constructor
                         MsnAppDirectoryService( QObject *parent = 0 );
    // The destructor
    virtual             ~MsnAppDirectoryService();

    // Return an application entry with a certain ID
    const Entry *        getEntryById(int entryId);
    // Return all entries
    const QList<Entry*>& getEntries() const;
    // Request a list of all services
    void                 queryServiceList( MsnAppDirectoryServiceType type );

  private:
    // Process server responses
    void                 parseSoapResult( SoapMessage *message );


  private:
    // A list of all received entries
    QList<Entry*>        entries_;
};

#endif
