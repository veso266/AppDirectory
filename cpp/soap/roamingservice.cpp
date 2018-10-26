/***************************************************************************
                          roamingservice.cpp -  description
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

#include "roamingservice.h"

#include "../../utils/kmessconfig.h"
#include "../../utils/kmessshared.h"
#include "../../utils/xmlfunctions.h"
#include "../../account.h"
#include "../../currentaccount.h"
#include "../../kmessdebug.h"
#include "soapmessage.h"

#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <KDateTime>

/**
 * @brief URL of the Storage Service
 */
#define SERVICE_URL_STORAGE_SERVICE  "https://storage.msn.com/storageservice/SchematizedStore.asmx"

// Constructor
RoamingService::RoamingService( QObject *parent )
: PassportLoginService( parent )
{
}



// Destructor
RoamingService::~RoamingService()
{
}



// Create the common header for this service
QString RoamingService::createCommonHeader() const
{
  QString header( "<StorageApplicationHeader xmlns=\"http://www.msn.com/webservices/storage/w10\">\n"
                    "<ApplicationID>Messenger Client 9.0</ApplicationID>\n"
                    "<Scenario>RoamingSeed</Scenario>\n"
                  "</StorageApplicationHeader>\n"
                  "<StorageUserHeader xmlns=\"http://www.msn.com/webservices/storage/w10\">\n"
                    "<Puid>0</Puid>\n"
                    "<TicketToken />\n" // Filled in by the base class
                  "</StorageUserHeader>\n" );
  return header;
}



// Get the profile for given contact
void RoamingService::getProfile( const QString& cid )
{
  cid_ = cid;
  QString body( "<GetProfile xmlns=\"http://www.msn.com/webservices/storage/w10\">\n"
                  "<profileHandle>\n"
                    "<Alias>\n"
                      "<Name>" + KMessShared::htmlEscape( cid ) + "</Name>\n"
                      "<NameSpace>MyCidStuff</NameSpace>\n"
                    "</Alias>\n"
                    "<RelationshipName>MyProfile</RelationshipName>\n"
                  "</profileHandle>\n"
                  "<profileAttributes>\n"
                    "<ResourceID>true</ResourceID>\n"
                    "<DateModified>true</DateModified>\n"
                    "<ExpressionProfileAttributes>\n"
                      "<ResourceID>true</ResourceID>\n"
                      "<DateModified>true</DateModified>\n"
                      "<DisplayName>true</DisplayName>\n"
                      "<DisplayNameLastModified>true</DisplayNameLastModified>\n"
                      "<PersonalStatus>true</PersonalStatus>\n"
                      "<PersonalStatusLastModified>true</PersonalStatusLastModified>\n"
                      "<StaticUserTilePublicURL>true</StaticUserTilePublicURL>\n"
                      "<Photo>true</Photo>\n"
                      "<Flags>true</Flags>\n"
                    "</ExpressionProfileAttributes>\n"
                  "</profileAttributes>\n"
                "</GetProfile>\n" );

  sendSecureRequest( new SoapMessage( SERVICE_URL_STORAGE_SERVICE,
                                      "http://www.msn.com/webservices/storage/w10/GetProfile",
                                      createCommonHeader(),
                                      body ),
                     "Storage" );
}



// Create a profile
void RoamingService::createProfile()
{
  QString body( "<CreateProfile xmlns=\"http://www.msn.com/webservices/storage/w10\">\n"
                "<profile>\n"
                "<ExpressionProfile>\n"
                "<PersonalStatus/>\n"
                "<RoleDefinitionName>ExpressionProfileDefault</RoleDefinitionName>\n"
                "</ExpressionProfile>\n"
                "</profile>\n"
                "</CreateProfile>" );

  sendSecureRequest( new SoapMessage( SERVICE_URL_STORAGE_SERVICE,
                                      "http://www.msn.com/webservices/storage/w10/CreateProfile",
                                      createCommonHeader(),
                                      body ),
                     "Storage" );
}



// update the profile of the current account
void RoamingService::updateProfile()
{
  CurrentAccount *currentAccount = CurrentAccount::instance();
  const QString currentPersonalMessage( currentAccount->getPersonalMessage( STRING_ORIGINAL ) );

  if( profileResourceId_.isEmpty() )
  {
    kWarning() << "Missing profileResourceId, not updating profile";
    return;
  }

  if( currentPersonalMessage == lastKnownPersonalMessage_ &&
      currentAccount->getFriendlyName( STRING_ORIGINAL ) == lastKnownFriendlyName_ )
  {
#ifdef KMESSDEBUG_ROAMINGSERVICE
    kDebug() << "Personal message and friendly name are same as on server; not uploading changes.";
#endif
    return;
  }

  // either the PSM or the friendly name has changed, so update them both.

  lastKnownPersonalMessage_ = currentPersonalMessage;
  lastKnownFriendlyName_    = currentAccount->getFriendlyName( STRING_ORIGINAL );

#ifdef KMESSDEBUG_ROAMINGSERVICE
  kDebug() << "Uploading personal message, profile ID:" << profileResourceId_;
#endif
  QString body( "<UpdateProfile xmlns=\"http://www.msn.com/webservices/storage/w10\">\n"
                "<profile>\n"
                "<ResourceID>" +
                  KMessShared::htmlEscape( profileResourceId_ ) +
                "</ResourceID>\n"
                "<ExpressionProfile>\n"
                "<FreeText>Update</FreeText>\n"
                "<DisplayName>" +
                  KMessShared::htmlEscape( lastKnownFriendlyName_ ) +
                "</DisplayName>\n"
                "<PersonalStatus>" +
                  KMessShared::htmlEscape( lastKnownPersonalMessage_ ) +
                "</PersonalStatus>\n"
                "<Flags>0</Flags>\n"
                "</ExpressionProfile>\n"
                "</profile>\n"
                "</UpdateProfile>\n");

  sendSecureRequest( new SoapMessage( SERVICE_URL_STORAGE_SERVICE,
                                      "http://www.msn.com/webservices/storage/w10/UpdateProfile",
                                      createCommonHeader(),
                                      body ),
                     "Storage" );
}



// update the display picture of the current acccount
void RoamingService::updateDisplayPicture()
{
  CurrentAccount *currentAccount = CurrentAccount::instance();
  const QString path( currentAccount->getPicturePath() );

  if( profileResourceId_.isEmpty() )
  {
    kWarning() << "Missing profileResourceId, not updating display picture";
    return;
  }

  // has the display picture really changed?
  if( path == lastKnownDisplayPicturePath_ )
  {
#ifdef KMESSDEBUG_ROAMINGSERVICE
    kDebug() << "Not updating display picture, already the same as on the server.";
#endif
    return;
  }
  lastKnownDisplayPicturePath_ = path;

  // see if we need to delete a previous picture
  if( ! displayPictureResourceId_.isEmpty() )
  {
#ifdef KMESSDEBUG_ROAMINGSERVICE
    kDebug() << "Deleting the previous display picture on the server.";
#endif
    deleteRelationships( "", displayPictureResourceId_ );
    deleteRelationships( profileResourceId_, displayPictureResourceId_ );
  }

  // upload new picture
#ifdef KMESSDEBUG_ROAMINGSERVICE
  kDebug() << "Uploading new display picture.";
#endif
  QFile file( currentAccount->getPicturePath() );
  if( file.open( QIODevice::ReadOnly ) )
  {
    QImageReader imageInfo( path );
    QFileInfo fileInfo( path );

    QString mimeType( "image/" );
    mimeType.append( imageInfo.format() );

    createDocument( fileInfo.baseName(), mimeType, file.readAll().toBase64() );
    file.close();
  }
  else
  {
    kWarning() << "Failed to read display picture from file" << currentAccount->getPicturePath();
  }
}



// Parse the SOAP fault
void RoamingService::parseSecureFault( SoapMessage *message )
{
  // Should never happen
  if( ! message->isFaultMessage() )
  {
    kWarning() << "Incorrect fault detected on incoming message";
    return;
  }

  // Get the type of error
  QString faultCode( message->getFaultCode() );
  QString errorCode( XmlFunctions::getNodeValue( message->getFault(), "detail/errorcode" ) );

  // See which fault we received
  if( faultCode == "soap:Client" && ! errorCode.isEmpty() )
  {
    // The profile does not exist
    if( errorCode == "ItemDoesNotExist" )
    {
#ifdef KMESSDEBUG_ROAMINGSERVICE
      kDebug() << "The profile of the user does not exist: creating one.";
#endif
      createProfile();
      return;
    }
    else if( errorCode == "ItemAlreadyExists" )
    {
      kWarning() << "The profile of the user already exists";
    }
  }
  else
  {
    kWarning() << "Invalid web service request:" << message->getFaultDescription();
  }
}



// The connection received the full response
void RoamingService::parseSecureResult( SoapMessage *message )
{
  QDomElement body( message->getBody().toElement() );
  QString resultName( body.firstChildElement().localName() );

  if( resultName == "GetProfileResponse" )
  {
#ifdef KMESSDEBUG_ROAMINGSERVICE
    kDebug() << "Getting profile response:" << profileResourceId_;
#endif
    processProfileResult( body );
  }
  else if( resultName == "UpdateProfileResponse" )
  {
#ifdef KMESSDEBUG_ROAMINGSERVICE
    kDebug() << "Updated profile:" << profileResourceId_;
#endif
    return;
  }
  else if( resultName == "CreateProfileResponse" )
  {
    profileResourceId_ = XmlFunctions::getNodeValue( body, "CreateProfileResponse/CreateProfileResult" );

#ifdef KMESSDEBUG_ROAMINGSERVICE
    kDebug() << "Created profile:" << profileResourceId_;
#endif
    // Empty profile was created, update it
    updateProfile();
  }
  else if( resultName == "DeleteRelationshipsResponse" )
  {
#ifdef KMESSDEBUG_ROAMINGSERVICE
    kDebug() << "Relationship deleted";
#endif

    return;
  }
  else if( resultName == "CreateDocumentResponse" )
  {
#ifdef KMESSDEBUG_ROAMINGSERVICE
    kDebug() << "Document created";
#endif

    displayPictureResourceId_ = XmlFunctions::getNodeValue( body, "CreateDocumentResponse/CreateDocumentResult" );
    // a new display picture was uploaded, create a relation with the profile
    createRelationships( profileResourceId_, displayPictureResourceId_ );
  }
  else if( resultName == "CreateRelationshipsResponse" )
  {
#ifdef KMESSDEBUG_ROAMINGSERVICE
    kDebug() << "Relationship created";
#endif
    return;
  }
  else
  {
    kWarning() << "Unknown response type" << resultName << "received from endpoint" << message->getEndPoint();
  }
}



// Process the getProfile response
void RoamingService::processProfileResult( const QDomElement &body )
{
  CurrentAccount *currentAccount = CurrentAccount::instance();

  const QDomNode expressionProfile( XmlFunctions::getNode( body, "GetProfileResponse/GetProfileResult/ExpressionProfile" ) );

  QString fullPictureUrl( XmlFunctions::getNodeValue( expressionProfile, "StaticUserTilePublicURL" ) );
  QString pictureLastModified( XmlFunctions::getNodeValue( expressionProfile, "Photo/DateModified" ) );
  QString pictureName( XmlFunctions::getNodeValue( expressionProfile, "Photo/Name" ) );
  // This message also contains DisplayName

  profileResourceId_        = XmlFunctions::getNodeValue( expressionProfile, "ResourceID" );
  displayPictureResourceId_ = XmlFunctions::getNodeValue( expressionProfile, "Photo/ResourceID" );
  lastKnownPersonalMessage_ = XmlFunctions::getNodeValue( expressionProfile, "PersonalStatus" );
  lastKnownFriendlyName_    = XmlFunctions::getNodeValue( expressionProfile, "DisplayName" );

  // Fix encoding of the friendly name and personal message
  lastKnownFriendlyName_    = QString::fromUtf8( lastKnownFriendlyName_.toAscii() );
  lastKnownPersonalMessage_ = QString::fromUtf8( lastKnownPersonalMessage_.toAscii() );

#ifdef KMESSDEBUG_ROAMINGSERVICE
  kDebug() << "Got profile result:" << profileResourceId_;
#endif

#ifdef KMESSDEBUG_ROAMINGSERVICE
  kDebug() << "Received friendly name from Storage:" << lastKnownFriendlyName_;
  kDebug() << "Received personalMessage from Storage:" << lastKnownPersonalMessage_;
  kDebug() << "Received displayPicture location from server:" << fullPictureUrl;
#endif

  // Don't update the details if they're not valid
  if( ! lastKnownFriendlyName_.isEmpty() )
  {
    emit receivedFriendlyName( lastKnownFriendlyName_ );
  }
  if( ! lastKnownPersonalMessage_.isEmpty() )
  {
    emit receivedPersonalMessage( lastKnownPersonalMessage_ );
  }

  // Only download the picture if it is enabled
  if( fullPictureUrl.isEmpty() || ! currentAccount->getShowPicture() )
  {
#ifdef KMESSDEBUG_ROAMINGSERVICE
    kDebug() << "Display picture disabled, not using received picture:" << fullPictureUrl;
#endif
    return;
  }

  // check if we don't have a picture with the same name (the msn server converts the image to jpeg and compresses it when storing,
  // so the hash changes and kmess can't see that the image is actually the same as one he uploaded earlier)
  const QString pictureDir( KMessConfig::instance()->getAccountDirectory( currentAccount->getHandle() ) + "/displaypics/" );
  const QString localPicture( pictureDir + pictureName + ".png" ); // accountpage always saves the images in png format
  if( QFile::exists( localPicture ) )
  {
#ifdef KMESSDEBUG_ROAMINGSERVICE
    kDebug() << "Have a local picture with the same name (" << localPicture << "), not downloading display picture";
#endif
    lastKnownDisplayPicturePath_ = localPicture;
    currentAccount->setPicturePath( localPicture );
    return;
  }

  // Check if the picture on the server is newer
  const QString displayPicturePath( currentAccount->getPicturePath( false /* don't fallback to the kmess default pic */ ) );
  QFileInfo info( displayPicturePath );

  // KDateTime is used to process the timestamp from the server,
  // because it also handles the timezone information
  if( displayPicturePath.isEmpty()
  ||  info.lastModified() < KDateTime::fromString( pictureLastModified, KDateTime::ISODate ).toClockTime().dateTime() )
  {
#ifdef KMESSDEBUG_ROAMINGSERVICE
    kDebug() << "Downloading display picture from storage";
#endif
    // Download the picture from the server into a temporary file
    QNetworkAccessManager *manager = new QNetworkAccessManager( this );

    connect( manager, SIGNAL(               finished(QNetworkReply*) ),
             this,    SLOT  ( receivedDisplayPicture(QNetworkReply*) ) );

    manager->get( QNetworkRequest( QUrl( fullPictureUrl ) ) );
  }
  else
  {
#ifdef KMESSDEBUG_ROAMINGSERVICE
	kDebug() << "DP path:" << displayPicturePath << " Last modification date:" << info.lastModified()
		<< "vs. server dp's date:" << KDateTime::fromString( pictureLastModified, KDateTime::ISODate ).toClockTime().dateTime();
#endif
  }

#ifdef KMESSDEBUG_ROAMINGSERVICE
    kDebug() << "Done.";
#endif
}



// Received the display picture from the server
void RoamingService::receivedDisplayPicture( QNetworkReply *reply )
{
  CurrentAccount *currentAccount = CurrentAccount::instance();

#ifdef KMESSDEBUG_ROAMINGSERVICE
  kDebug() << "Saving received display picture of size" << reply->size();
#endif

  if( reply->size() == 0 )
  {
#ifdef KMESSDEBUG_ROAMINGSERVICE
    kDebug() << "Received empty content, ignoring response.";
#endif
    return;
  }

  if( ! reply->header( QNetworkRequest::LocationHeader ).toString().isEmpty() )
  {
#ifdef KMESSDEBUG_ROAMINGSERVICE
    kDebug() << "Received Location: header, ignoring response.";
    kDebug() << "Location:" << reply->header( QNetworkRequest::LocationHeader ).toString();
#endif
    return;
  }

  // We need to also delete the calling QNetworkAccessManager to avoid memory leaks
  QNetworkAccessManager *manager = reply->manager();

  // check if the picture dir exists
  QDir accountDir( KMessConfig::instance()->getAccountDirectory( currentAccount->getHandle() ) );
  if( ! accountDir.exists( "displaypics" ) )
  {
    accountDir.mkdir( "displaypics" );
  }

  const QString pictureDir( KMessConfig::instance()->getAccountDirectory( currentAccount->getHandle() ) +
                            "/displaypics/" );
  QString pictureName( "temporary-picture.dat" );
  const QString tempPicturePath( pictureDir + pictureName );

  QFile file( tempPicturePath );

  // If the file can't be saved, do nothing
  if( ! file.open( QIODevice::WriteOnly ) )
  {
    reply->deleteLater();
    if( manager != 0 )
    {
      manager->deleteLater();
    }
    return;
  }

  // Save the picture on file, we'll move it later to a good place
  file.write( reply->readAll() );
  file.flush();
  file.close();

  reply->deleteLater();

  if( manager != 0 )
  {
    manager->deleteLater();
    manager = 0;
  }

  // Retrieve the picture's hash
  QString msnObjectHash( KMessShared::generateFileHash( tempPicturePath ).toBase64() );
  const QString safeMsnObjectHash( msnObjectHash.replace( QRegExp( "[^a-zA-Z0-9+=]"), "_" ) );

  // Find out the image format
  QImageReader imageInfo( tempPicturePath );

  // Save the new display picture with its hash as name, to ensure correct caching
  pictureName = safeMsnObjectHash + "." + imageInfo.format();

  QDir pictureFolder( pictureDir );

  // Do not overwrite identical pictures
  if( ! pictureFolder.exists( pictureName )
  &&  ! pictureFolder.rename( tempPicturePath, pictureName ) )
  {
    kWarning() << "The display picture file could not be renamed from" << tempPicturePath << "to" << pictureName << ".";
  }
#ifdef KMESSDEBUG_ROAMINGSERVICE
  else
  {
    kDebug() << "Saved display picture as" << ( pictureDir + pictureName );
  }
#endif

  lastKnownDisplayPicturePath_ = pictureDir + pictureName;
  currentAccount->setPicturePath( pictureDir + pictureName );

  emit receivedDisplayPicture( lastKnownDisplayPicturePath_ );
}



// create a document on the server (upload a display picture)
void RoamingService::createDocument( const QString& name, const QString& mimeType, const QString& data )
{
  QString body( "<CreateDocument xmlns=\"http://www.msn.com/webservices/storage/w10\">\n"
                  "<parentHandle>\n"
                    "<RelationshipName>/UserTiles</RelationshipName>\n"
                    "<Alias>\n"
                      "<Name>" + KMessShared::htmlEscape( cid_ ) + "</Name>\n"
                      "<NameSpace>MyCidStuff</NameSpace>\n"
                    "</Alias>\n"
                  "</parentHandle>\n"
                  "<document xsi:type=\"Photo\">\n"
                    "<Name>" + KMessShared::htmlEscape( name ) + "</Name>\n"
                    "<DocumentStreams>\n"
                      "<DocumentStream xsi:type=\"PhotoStream\">\n"
                        "<DocumentStreamType>UserTileStatic</DocumentStreamType>\n"
                        "<MimeType>" + KMessShared::htmlEscape( mimeType ) + "</MimeType>\n"
                        "<Data>" + KMessShared::htmlEscape( data ) + "</Data>\n"
                        "<DataSize>0</DataSize>\n"
                      "</DocumentStream>\n"
                    "</DocumentStreams>\n"
                  "</document>\n"
                  "<relationshipName>Messenger User Tile</relationshipName>\n"
                "</CreateDocument>\n" );

  sendSecureRequest( new SoapMessage( SERVICE_URL_STORAGE_SERVICE,
                                      "http://www.msn.com/webservices/storage/w10/CreateDocument",
                                      createCommonHeader(),
                                      body ),
                     "Storage" );
}



// list the display pictures on the server (gets displayPictureResourceId), this method is currently not used
void RoamingService::findDocuments()
{
  QString body( "<FindDocuments xmlns=\"http://www.msn.com/webservices/storage/w10\">\n"
                  "<objectHandle>\n"
                    "<RelationshipName>/UserTiles</RelationshipName>\n"
                    "<Alias>\n"
                      "<Name>" + KMessShared::htmlEscape( cid_ ) + "</Name>\n"
                      "<NameSpace>MyCidStuff</NameSpace>\n"
                    "</Alias>\n"
                  "</objectHandle>\n"
                  "<documentAttributes>\n"
                    "<ResourceID>true</ResourceID>\n"
                    "<Name>true</Name>\n"
                  "</documentAttributes>\n"
                  "<documentFilter>\n"
                    "<FilterAttributes>None</FilterAttributes>\n"
                  "</documentFilter>\n"
                  "<documentSort>\n"
                    "<SortBy>DateModified</SortBy>\n"
                  "</documentSort>\n"
                  "<findContext>\n"
                    "<FindMethod>Default</FindMethod>\n"
                    "<ChunkSize>25</ChunkSize>\n"
                  "</findContext>\n"
                "</FindDocuments>\n" );

  sendSecureRequest( new SoapMessage( SERVICE_URL_STORAGE_SERVICE,
                                      "http://www.msn.com/webservices/storage/w10/FindDocuments",
                                      createCommonHeader(),
                                      body ),
                     "Storage" );
}



// create a relationship between two resources on the server
void RoamingService::createRelationships( const QString& source_rid, const QString& target_rid )
{
  QString body( "<CreateRelationships xmlns=\"http://www.msn.com/webservices/storage/w10\">\n"
                  "<relationships>\n"
                    "<Relationship>\n"
                      "<SourceID>" + KMessShared::htmlEscape( source_rid ) + "</SourceID>\n"
                      "<SourceType>SubProfile</SourceType>\n"
                      "<TargetID>" + KMessShared::htmlEscape( target_rid ) + "</TargetID>\n"
                      "<TargetType>Photo</TargetType>\n"
                      "<RelationshipName>ProfilePhoto</RelationshipName>\n"
                    "</Relationship>\n"
                  "</relationships>\n"
                "</CreateRelationships>\n" );

  sendSecureRequest( new SoapMessage( SERVICE_URL_STORAGE_SERVICE,
                                      "http://www.msn.com/webservices/storage/w10/CreateRelationships",
                                      createCommonHeader(),
                                      body ),
                     "Storage" );
}



// delete a relationship between two resources on the server
void RoamingService::deleteRelationships( const QString& source_rid, const QString& target_rid )
{
  QString sourceHandle;

  if( source_rid.isEmpty() )
  {
    sourceHandle = "<RelationshipName>/UserTiles</RelationshipName>\n"
                   "<Alias>\n"
                     "<Name>" + KMessShared::htmlEscape( cid_ ) + "</Name>\n"
                     "<NameSpace>MyCidStuff</NameSpace>\n"
                   "</Alias>\n";
  }
  else
  {
    sourceHandle = "<ResourceID>" + KMessShared::htmlEscape( source_rid ) + "</ResourceID>\n";
  }

  QString body( "<DeleteRelationships xmlns=\"http://www.msn.com/webservices/storage/w10\">\n"
                  "<sourceHandle>\n" + sourceHandle + "</sourceHandle>\n"
                  "<targetHandles>\n"
                    "<ObjectHandle>\n"
                      "<ResourceID>" + KMessShared::htmlEscape( target_rid ) + "</ResourceID>\n"
                    "</ObjectHandle>\n"
                  "</targetHandles>\n"
                "</DeleteRelationships>\n" );

   sendSecureRequest( new SoapMessage( SERVICE_URL_STORAGE_SERVICE,
                                      "http://www.msn.com/webservices/storage/w10/DeleteRelationships",
                                      createCommonHeader(),
                                      body ),
                     "Storage" );
}



#include "roamingservice.moc"
