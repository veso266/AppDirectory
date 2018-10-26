/***************************************************************************
                          msnappdirectoryservice.cpp -  description
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

#include "msnappdirectoryservice.h"

#include "../../utils/xmlfunctions.h"
#include "../../kmessdebug.h"
#include "httpsoapconnection.h"
#include "soapmessage.h"


#ifdef KMESSDEBUG_APPDIRECTORYSERVICE
#define KMESSDEBUG_APPDIRECTORYSERVICE_GENERAL
#endif


/**
 * @brief URL of the Application Directory Service
 */
#define SERVICE_URL_APPDIRSERVICE  "http://appdirectory.messenger.msn.com/AppDirectory/AppDirectory.asmx"



// Constructor
MsnAppDirectoryService::MsnAppDirectoryService( QObject *parent )
  : HttpSoapConnection( parent )
{
#ifdef KMESSDEBUG_APPDIRECTORYSERVICE_GENERAL
  kDebug() << "CREATED.";
#endif

  setObjectName( "MsnAppDirectoryService" );
}



// Destructor
MsnAppDirectoryService::~MsnAppDirectoryService()
{
  qDeleteAll( entries_ );

#ifdef KMESSDEBUG_APPDIRECTORYSERVICE_GENERAL
  kDebug() << "DESTROYED.";
#endif
}



// Return an application entry with a certain ID
const MsnAppDirectoryService::Entry * MsnAppDirectoryService::getEntryById( int entryId )
{
  foreach( Entry *entry, entries_ )
  {
    if( entry->entryId == entryId )
    {
      return entry;
    }
  }

  return 0;
}



// Return all entries
const QList<MsnAppDirectoryService::Entry*>& MsnAppDirectoryService::getEntries() const
{
  return entries_;
}



// A soap request finished
void MsnAppDirectoryService::parseSoapResult( SoapMessage *message )
{
#ifdef KMESSDEBUG_APPDIRECTORYSERVICE_GENERAL
  kDebug() << "Got query response";
#endif

  // Parse the list of result entries
  QDomNode     dataSet( XmlFunctions::getNode( message->getBody(), "diffgram/NewDataSet" ) );
  QDomNodeList entries( dataSet.childNodes() );

  for( int i = 0; i < entries.count(); i++ )
  {
    QDomNode entryProperties( entries.item( i ) );

    int entryId = XmlFunctions::getNodeValue( entryProperties, "EntryID" ).toInt();
    if( getEntryById( entryId ) != 0 )
    {
      // Ignore entries that are already present
      continue;
    }

    // Fill the values
    Entry *entry = new Entry;
    entry->entryId         = entryId;
    entry->subscriptionUrl = XmlFunctions::getNodeValue( entryProperties, "SubscriptionURL" );
    entry->error           = XmlFunctions::getNodeValue( entryProperties, "Error"           );
    entry->locale          = XmlFunctions::getNodeValue( entryProperties, "Locale"          );
    entry->sequence        = XmlFunctions::getNodeValue( entryProperties, "Sequence"        );
    entry->name            = XmlFunctions::getNodeValue( entryProperties, "Name"            );
    entry->description     = XmlFunctions::getNodeValue( entryProperties, "Description"     );
    entry->url             = XmlFunctions::getNodeValue( entryProperties, "URL"             );
    entry->iconUrl         = XmlFunctions::getNodeValue( entryProperties, "IconURL"         );
    entry->appIconUrl      = XmlFunctions::getNodeValue( entryProperties, "AppIconURL"      );
    entry->type            = XmlFunctions::getNodeValue( entryProperties, "Type"            );
    entry->location        = XmlFunctions::getNodeValue( entryProperties, "Location"        );
    entry->clientVersion   = XmlFunctions::getNodeValue( entryProperties, "ClientVersion"   );
    entry->page            = XmlFunctions::getNodeValue( entryProperties, "Page"            ).toInt();
    entry->categoryId      = XmlFunctions::getNodeValue( entryProperties, "CategoryID"      ).toInt();
    entry->passportSiteId  = XmlFunctions::getNodeValue( entryProperties, "PassportSiteID"  ).toInt();
    entry->height          = XmlFunctions::getNodeValue( entryProperties, "Height"          ).toInt();
    entry->width           = XmlFunctions::getNodeValue( entryProperties, "Width"           ).toInt();
    entry->minUsers        = XmlFunctions::getNodeValue( entryProperties, "MinUsers"        ).toInt();
    entry->maxUsers        = XmlFunctions::getNodeValue( entryProperties, "MaxUsers"        ).toInt();
    entry->maxPacketRate   = XmlFunctions::getNodeValue( entryProperties, "MaxPacketRate"   ).toInt();
    entry->appType         = XmlFunctions::getNodeValue( entryProperties, "AppType"         ).toInt();
    entry->kids            = XmlFunctions::getNodeValue( entryProperties, "Kids"            ) == "1";
    entry->enableIp        = XmlFunctions::getNodeValue( entryProperties, "EnableIP"        ) == "True";
    entry->activeX         = XmlFunctions::getNodeValue( entryProperties, "ActiveX"         ) == "True";
    entry->sendFile        = XmlFunctions::getNodeValue( entryProperties, "SendFile"        ) == "True";
    entry->receiveIM       = XmlFunctions::getNodeValue( entryProperties, "ReceiveIM"       ) == "True";
    entry->replaceIM       = XmlFunctions::getNodeValue( entryProperties, "ReplaceIM"       ) == "True";
    entry->windows         = XmlFunctions::getNodeValue( entryProperties, "Windows"         ) == "True";
    entry->userProperties  = XmlFunctions::getNodeValue( entryProperties, "UserProperties"  ) == "True";
    entry->hidden          = XmlFunctions::getNodeValue( entryProperties, "Hidden"          ) == "True";

#ifdef KMESSDEBUG_APPDIRECTORYSERVICE_GENERAL
    kDebug() << "Received entry " << entry->name << ".";
#endif

    // Add to the list
    entries_.append( entry );
  }

#ifdef KMESSDEBUG_APPDIRECTORYSERVICE_GENERAL
  kDebug() << "Emitting that request was successful";
#endif
}



// Request a list of all services
void MsnAppDirectoryService::queryServiceList( MsnAppDirectoryServiceType type )
{
  Q_UNUSED( type );

#ifdef KMESSDEBUG_APPDIRECTORYSERVICE_GENERAL
  kDebug() << "Querying service list";
#endif

  QString body( "<GetFilteredDataSet2 xmlns=\"http://www.msn.com/webservices/Messenger/Client\">\n"
                "  <locale>en-us</locale>\n"
                "  <Page>1</Page>\n"
                "  <Kids>-1</Kids>\n"
                "  <AppType>0</AppType>\n"
                "</GetFilteredDataSet2>" );

  sendRequest( new SoapMessage( SERVICE_URL_APPDIRSERVICE,
                                "http://www.msn.com/webservices/Messenger/Client/GetFilteredDataSet2",
                                QString(),
                                body ) );
}



#include "msnappdirectoryservice.moc"
