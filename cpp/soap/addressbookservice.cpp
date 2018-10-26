/***************************************************************************
                          addressbookservice.cpp
                             -------------------
    begin                : Fri July 25 2008
    copyright            : (C) 2008 by Antonio Nastasi
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

#include "addressbookservice.h"

#include "../../contact/contact.h"
#include "../../utils/kmessshared.h"
#include "../../utils/xmlfunctions.h"
#include "../../currentaccount.h"
#include "../../kmessdebug.h"
#include "soapmessage.h"

#include <KLocale>


#ifdef KMESSDEBUG_HTTPSOAPCONNECTION
  #define KMESSDEBUG_ADDRESSBOOKSERVICE
#endif


/**
 * @brief URL of the Address Book Service
 */
#define SERVICE_URL_ADDRESSBOOK           "https://omega.contacts.msn.com/abservice/abservice.asmx"

/**
 * @brief URL of the Address Book Sharing Service
 */
#define SERVICE_URL_ADDRESSBOOK_SHARING   "https://omega.contacts.msn.com/abservice/SharingService.asmx"



/**
 * @brief The constructor
 */
AddressBookService::AddressBookService( QObject *parent )
: PassportLoginService( parent )
{
}



/**
 * @brief The destructor
 */
AddressBookService::~AddressBookService()
{
}


/**
 * @brief Add a new or already known contact to the Address Book.
 *
 * To add support for Yahoo contacts it may be needed to use the FQY NS command,
 * as it is used to determine the real type (or types!) of an address.
 */
void AddressBookService::addContact( const QString &handle, const QStringList& groupsId, bool alreadyExists )
{
  // TODO Add support to yahoo contacts

  QString body( "<ABContactAdd xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
                "  <abId>00000000-0000-0000-0000-000000000000</abId>\n"
                "  <contacts>\n"
                "    <Contact xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
                "      <contactInfo>\n" );
  if( ! alreadyExists )
  {
    body +=     "        <contactType>Regular</contactType>\n";
  }

  body +=       "        <passportName>" + handle + "</passportName>\n"
                "        <isMessengerUser>true</isMessengerUser>\n"
                "        <MessengerMemberInfo>\n"
                "          <DisplayName />\n"
                "        </MessengerMemberInfo>\n"
                "      </contactInfo>\n"
                "    </Contact>\n"
                "  </contacts>\n";
  if( ! alreadyExists )
  {
    body +=     "  <options>\n"
                "    <EnableAllowListManagement>true</EnableAllowListManagement>\n"
                "  </options>\n";
  }

  body +=       "</ABContactAdd>";

  MessageData data;
  data.type  = "ContactAdd";
  data.value = QStringList() << handle << groupsId;

  sendSecureRequest( new SoapMessage( SERVICE_URL_ADDRESSBOOK,
                                      "http://www.msn.com/webservices/AddressBook/ABContactAdd",
                                      createCommonHeader( "ContactSave" ),
                                      body,
                                      data ),
                     "Contacts" );
}



void AddressBookService::addContactToGroup( const QString &contactId, const QString &groupId )
{
  QString body( "<ABGroupContactAdd xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
                "  <abId>00000000-0000-0000-0000-000000000000</abId>\n"
                "  <groupFilter>\n"
                "    <groupIds>\n"
                "      <guid>" + groupId + "</guid>\n"
                "    </groupIds>\n"
                "  </groupFilter>\n"
                "  <contacts>\n"
                "    <Contact>\n"
                "      <contactId>" + contactId + "</contactId>\n"
                "    </Contact>\n"
                "  </contacts>\n"
                "</ABGroupContactAdd>" );

  MessageData data;
  data.type  = "ContactAddToGroup";
  data.value = groupId;

  sendSecureRequest( new SoapMessage( SERVICE_URL_ADDRESSBOOK,
                                      "http://www.msn.com/webservices/AddressBook/ABGroupContactAdd",
                                      createCommonHeader( "GroupSave" ),
                                      body,
                                      data ),
                     "Contacts" );
}



void AddressBookService::addGroup( const QString &name )
{
  QString body( "<ABGroupAdd xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
                "  <abId>00000000-0000-0000-0000-000000000000</abId>\n"
                "  <groupAddOptions>\n"
                "    <fRenameOnMsgrConflict>false</fRenameOnMsgrConflict>\n"
                "  </groupAddOptions>\n"
                "  <groupInfo>\n"
                "    <GroupInfo>\n"
                "      <name>" + KMessShared::htmlEscape( name ) + "</name>\n"
                "      <groupType>C8529CE2-6EAD-434d-881F-341E17DB3FF8</groupType>\n"
                "      <fMessenger>false</fMessenger>\n"
                "      <annotations>\n"
                "        <Annotation>\n"
                "          <Name>MSN.IM.Display</Name>\n"
                "          <Value>1</Value>\n"
                "        </Annotation>\n"
                "      </annotations>\n"
                "    </GroupInfo>\n"
                "  </groupInfo>\n"
                "</ABGroupAdd>" );

  MessageData data;
  data.type  = "GroupAdd";
  data.value = name;

  sendSecureRequest( new SoapMessage( SERVICE_URL_ADDRESSBOOK,
                                      "http://www.msn.com/webservices/AddressBook/ABGroupAdd",
                                      createCommonHeader( "GroupSave" ),
                                      body,
                                      data ),
                     "Contacts" );
}



// Block contact
void AddressBookService::blockContact( const QString &handle )
{
  SoapMessage *message = getMembershipListUpdate( handle, "Block", true );

  MessageData data;
  data.type  = "ContactBlock";
  data.value = QStringList() << handle << "BL";

  message->setData( data );

  sendSecureRequest( message, "Contacts" );
}



/**
 * @brief Request creation of a new address book.
 *
 * Newly created accounts have no address book. This method
 * requests the creation of a new one.
 */
void AddressBookService::createAddressBook()
{
  QString body( "<ABAdd xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
                "  <abInfo>\n"
                "    <name/>\n"
                "    <ownerPuid>0</ownerPuid>\n"
                "    <ownerEmail>" + CurrentAccount::instance()->getHandle() + "</ownerEmail>\n"
                "    <fDefault>true</fDefault>\n"
                "  </abInfo>\n"
                "</ABAdd>" );

  sendSecureRequest( new SoapMessage( SERVICE_URL_ADDRESSBOOK,
                                      "http://www.msn.com/webservices/AddressBook/ABAdd",
                                      createCommonHeader(),
                                      body ),
                     "Contacts" );
}



/**
 * @brief Create the common header for the soap requests.
 *
 * @param partnerScenario the field for the PartnerScenario tag
 */
QString AddressBookService::createCommonHeader( const QString partnerScenario )
{
  return "<ABApplicationHeader xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
         "  <ApplicationId xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
         "    996CDE1E-AA53-4477-B943-2BE802EA6166\n"
         "  </ApplicationId>\n"
         "  <IsMigration xmlns=\"http://www.msn.com/webservices/AddressBook\">false</IsMigration>\n"
         "  <PartnerScenario xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
         + partnerScenario +
         "  </PartnerScenario>\n"
         "</ABApplicationHeader>\n"
         "<ABAuthHeader xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
         "  <ManagedGroupRequest xmlns=\"http://www.msn.com/webservices/AddressBook\">false</ManagedGroupRequest>\n"
         "  <TicketToken />\n" // Gets filled in by the base class
         "</ABAuthHeader>";
}



/**
 * @brief Put the Soap action to update the information about contact.
 *
 * @param property the property that has changed (see ContactProperty in addressbookservice.h)
 * @param newValue the new value for that property.
 */
void AddressBookService::contactUpdate( ContactProperty property, const QString &newValue )
{
  QString propertyString;
  QString propertiesChanged;

  switch( property )
  {
    case PROPERTY_FRIENDLYNAME:
      propertyString = "        <displayName>" + KMessShared::htmlEscape( newValue ) + "</displayName>\n";
      propertiesChanged = "DisplayName";
      break;

    case PROPERTY_PRIVACY:
      propertyString = (  "        <annotations>\n"
                          "         <Annotation>\n"
                          "           <Name>MSN.IM.BLP</Name>\n"
                          "           <Value>" + newValue + "</Value>\n"
                          "         </Annotation>\n"
                          "        </annotations>\n");
      propertiesChanged = "Annotation";
      break;

    default:
      return;
  }

  QString body( "<ABContactUpdate xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
                "  <abId>00000000-0000-0000-0000-000000000000</abId>\n"
                "  <contacts>\n"
                "    <Contact xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
                "      <contactInfo>\n"
                "        <contactType>Me</contactType>\n"
                        + propertyString +
                "      </contactInfo>\n"
                "      <propertiesChanged>" + propertiesChanged + "</propertiesChanged>\n"
                "    </Contact>\n"
                "  </contacts>\n"
                "</ABContactUpdate>" );

  sendSecureRequest( new SoapMessage( SERVICE_URL_ADDRESSBOOK,
                                      "http://www.msn.com/webservices/AddressBook/ABContactUpdate",
                                      createCommonHeader( "Timer" ),
                                      body ),
                     "Contacts" );
}



void AddressBookService::deleteContact( const QString &contactId )
{
  QString body( "<ABContactDelete xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
                "  <abId>00000000-0000-0000-0000-000000000000</abId>\n"
                "  <contacts>\n"
                "    <Contact>\n"
                "      <contactId>" + contactId + "</contactId>\n"
                "    </Contact>\n"
                "  </contacts>\n"
                "</ABContactDelete>" );

  MessageData data;
  data.type  = "ContactDelete";
  data.value = contactId;

  sendSecureRequest( new SoapMessage( SERVICE_URL_ADDRESSBOOK,
                                      "http://www.msn.com/webservices/AddressBook/ABContactDelete",
                                      createCommonHeader( "Timer" ),
                                      body,
                                      data ),
                     "Contacts" );
}



void AddressBookService::deleteContactFromGroup( const QString &contactId, const QString &groupId )
{
  QString body( "<ABGroupContactDelete xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
                "  <abId>00000000-0000-0000-0000-000000000000</abId>\n"
                "  <groupFilter>\n"
                "    <groupIds>\n"
                "      <guid>" + groupId + "</guid>\n"
                "    </groupIds>\n"
                "  </groupFilter>\n"
                "  <contacts>\n"
                "    <Contact>\n"
                "      <contactId>" + contactId + "</contactId>\n"
                "    </Contact>\n"
                "  </contacts>\n"
                "</ABGroupContactDelete>" );

  MessageData data;
  data.type  = "ContactDeleteFromGroup";
  data.value = QStringList() << contactId << groupId;

  sendSecureRequest( new SoapMessage( SERVICE_URL_ADDRESSBOOK,
                                      "http://www.msn.com/webservices/AddressBook/ABGroupContactDelete",
                                      createCommonHeader( "GroupSave" ),
                                      body,
                                      data ),
                     "Contacts" );
}



void AddressBookService::deleteGroup( const QString &groupId )
{
  QString body( "<ABGroupDelete xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
                "  <abId>00000000-0000-0000-0000-000000000000</abId>\n"
                "  <groupFilter>\n"
                "    <groupIds>\n"
                "      <guid>" + groupId + "</guid>\n"
                "    </groupIds>\n"
                "  </groupFilter>\n"
                "</ABGroupDelete>" );

  MessageData data;
  data.type  = "GroupDelete";
  data.value = groupId;

  sendSecureRequest( new SoapMessage( SERVICE_URL_ADDRESSBOOK,
                                      "http://www.msn.com/webservices/AddressBook/ABGroupDelete",
                                      createCommonHeader( "GroupSave" ),
                                      body,
                                      data ),
                     "Contacts" );
}



// marks a contact Id as a messenger user (isMessengerUser = true )
// Hotmail contacts will appear on our AB (but they'll be isMessengerUser = false)
// if these contacts are added to our Messenger list we need to update them as 
// valid messenger users. AB service won't do this for us, apparently.
void AddressBookService::markMessengerUser( const QString &contactId )
{
  QString body( "<ABContactUpdate xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
                "  <abId>00000000-0000-0000-0000-000000000000</abId>\n"
                "  <contacts>\n"
                "    <Contact xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
                "      <contactId>" + contactId + "</contactId>\n"
                "      <contactInfo>\n"
                "          <isMessengerUser>true</isMessengerUser>\n"
                "      </contactInfo>\n"
                "      <propertiesChanged>IsMessengerUser</propertiesChanged>\n"
                "    </Contact>\n"
                "  </contacts>\n"
                "</ABContactUpdate>" );

  sendSecureRequest( new SoapMessage( SERVICE_URL_ADDRESSBOOK,
                                      "http://www.msn.com/webservices/AddressBook/ABContactUpdate",
                                      createCommonHeader( "Timer" ),
                                      body ),
                     "Contacts" );
}



void AddressBookService::renameGroup( const QString &groupId, const QString &name )
{
  QString body( "<ABGroupUpdate xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
                "  <abId>00000000-0000-0000-0000-000000000000</abId>\n"
                "  <groups>\n"
                "    <Group>\n"
                "      <groupId>" + groupId + "</groupId>\n"
                "      <groupInfo>\n"
                "        <name>" + KMessShared::htmlEscape( name ) + "</name>\n"
                "      </groupInfo>\n"
                "      <propertiesChanged>GroupName</propertiesChanged>\n"
                "    </Group>\n"
                "  </groups>\n"
                "</ABGroupUpdate>" );

  MessageData data;
  data.type  = "GroupRename";
  data.value = QStringList() << groupId << name;

  sendSecureRequest( new SoapMessage( SERVICE_URL_ADDRESSBOOK,
                                      "http://www.msn.com/webservices/AddressBook/ABGroupUpdate",
                                      createCommonHeader( "GroupSave" ),
                                      body,
                                      data ),
                     "Contacts" );
}



/**
 * Remove a user from the Pending list.
 */
void AddressBookService::removePending( const QString &handle )
{
  SoapMessage *msg = getMembershipListUpdate( handle, "Pending", false );
  sendSecureRequest( msg, "Contacts" );
}



/**
 * @brief Retrieve the address book.
 *
 * I've read about a possible small alternative to the current "ABFindAll".
 * It is:
 * <code>
GET email@domain.com/LiveContacts/?Filter=LiveContacts(Contact(ID,IMAddress)) HTTP/1.1
Host: livecontacts.live.com
Cookie: MSPAuth=..
</code>
 * It goes on with the headers as usual. Must be tested, I don't know if it works, or
 * if the response is identical to that of the current AbFindAll.
 *
 * @param fromTimestamp A timestamp to retrieve only AB changes after a certain moment.
 */
void AddressBookService::retrieveAddressBook( const QString &fromTimestamp )
{
  QString body( "<ABFindAll xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
                "  <abId>00000000-0000-0000-0000-000000000000</abId>\n"
                "  <abView>Full</abView>\n" );
  if( fromTimestamp.isEmpty() )
  {
    body +=     "  <deltasOnly>false</deltasOnly>\n"
                "  <lastChange>0001-01-01T00:00:00.0000000-08:00</lastChange>\n";
  }
  else
  {
    body +=     "  <deltasOnly>true</deltasOnly>\n"
                "  <lastChange>" + fromTimestamp + "</lastChange>\n";
  }

  body +=       "</ABFindAll>";

  sendSecureRequest( new SoapMessage( SERVICE_URL_ADDRESSBOOK,
                                      "http://www.msn.com/webservices/AddressBook/ABFindAll",
                                      createCommonHeader(),
                                      body ),
                     "Contacts" );
}



/**
 * @brief Retrieve the membership lists
 */
void AddressBookService::retrieveMembershipLists()
{
  QString body( "<FindMembership xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
                "  <serviceFilter xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
                "    <Types xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
                "      <ServiceType xmlns=\"http://www.msn.com/webservices/AddressBook\">Messenger</ServiceType>\n"
// TODO: Adding more WLM features may require retrieval of the membership lists for these services
/*
                "      <ServiceType xmlns=\"http://www.msn.com/webservices/AddressBook\">Invitation</ServiceType>\n"
                "      <ServiceType xmlns=\"http://www.msn.com/webservices/AddressBook\">SocialNetwork</ServiceType>\n"
                "      <ServiceType xmlns=\"http://www.msn.com/webservices/AddressBook\">Space</ServiceType>\n"
                "      <ServiceType xmlns=\"http://www.msn.com/webservices/AddressBook\">Profile</ServiceType>\n"
*/
                "    </Types>\n"
                "  </serviceFilter>\n"
                "</FindMembership>" );

  sendSecureRequest( new SoapMessage( SERVICE_URL_ADDRESSBOOK_SHARING,
                                      "http://www.msn.com/webservices/AddressBook/FindMembership",
                                      createCommonHeader(),
                                      body ),
                     "Contacts" );
}



/**
 * @brief Retrieve the Dynamic Items: Gleams
 */
void AddressBookService::retrieveGleams()
{
  QString body( "<ABFindAll xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
                "  <abId>00000000-0000-0000-0000-000000000000</abId>\n"
                "  <abView>Full</abView>\n"
                "  <deltasOnly>false</deltasOnly>\n"
                "  <lastChange>0001-01-01T00:00:00.0000000-08:00</lastChange>\n"
                "  <dynamicItemView>Gleam</dynamicItemView>\n"
                "  <dynamicItemLastChange>0001-01-01T00:00:00.0000000-08:00</dynamicItemLastChange>\n"
                "</ABFindAll>" );

  sendSecureRequest( new SoapMessage( SERVICE_URL_ADDRESSBOOK,
                                      "http://www.msn.com/webservices/AddressBook/ABFindAll",
                                      createCommonHeader(),
                                      body ),
                     "Contacts" );
}



// Parse the address book
void AddressBookService::parseAddressBook( const QDomElement &body )
{
#ifdef KMESSDEBUG_ADDRESSBOOKSERVICE
  kDebug() << "Parsing address book.";

  int debugGroupCount = 0;
#endif

  // Parse the adress book lists
  // First parse the one containing the groups
  const QDomNodeList groups( body.elementsByTagName( "Group" ) );

  for( int index = 0; index < groups.count(); index++ )
  {
    const QDomNode group( groups.item( index ) );

    const QString groupId(                 XmlFunctions::getNodeValue( group, "groupId"        ) );
    const QString    name( textNodeDecode( XmlFunctions::getNodeValue( group, "groupInfo/name" ) ) );

    if( groupId.isEmpty() || name.isEmpty() )
    {
      continue;
    }

#ifdef KMESSDEBUG_ADDRESSBOOKSERVICE
    ++debugGroupCount;
#endif
    emit gotGroup( groupId, name );
  }

  // Then parse the list which contains the contacts
  const QDomNodeList contactsNode( body.elementsByTagName( "Contact" ) );
  QList< QHash<QString,QVariant> > contacts;
  QString handle;
  QString information;
  QStringList guidList;

  for( int index = 0; index < contactsNode.count(); index++ )
  {
    const QDomNode contact( contactsNode.item( index ) );
    const QDomNode contactInfo( XmlFunctions::getNode( contact, "contactInfo" ) );

    // Get some user information
    const QString contactType( XmlFunctions::getNodeValue( contactInfo, "contactType" ) );
    const QString friendlyName( textNodeDecode( XmlFunctions::getNodeValue( contactInfo, "displayName" ) ));

    // Check if the contact type is Me: it has the information about personal profile
    if( contactType == "Me" )
    {
      const QString cid( XmlFunctions::getNodeValue( contactInfo, "CID" ) );

      // Search for "<annotations>...<Annotation><name>MSN.IM.BLP<name><value>VALUE<value><Annotation>.."
      const QDomNodeList annotations( contactInfo.toElement().elementsByTagName( "Annotation" ) );
      int blp = 0; // = 0 fixes compile time warning
      for( int index2 = 0; index2 < annotations.count(); index2++ )
      {
        const QDomNode annotation( annotations.at( index2 ) );

        if( XmlFunctions::getNodeValue( annotation, "Name" ) == "MSN.IM.BLP" )
        {
          // Set BLP argument
          blp = XmlFunctions::getNodeValue( annotation, "Value" ).toInt();
          break;
        }
      }

      emit gotPersonalInformation( cid, blp );
      continue;
    }

      // If the contact is not in our Messenger contact list, skip it.
      // adam (24 Jul 10): can't skip these users. non-messenger (i.e, hotmail only)
      // users will appear on our AB list. we might need to add them one day. if that
      // happens we'll need a Contact instance to refer to.
//     if( XmlFunctions::getNodeValue( contactInfo, "isMessengerUser" ) == "false" )
//     {
//       continue;
//     }

    // Store all information about contact in one qhash
    QHash<QString, QVariant> contactInformations;

    if( XmlFunctions::getNodeValue( contactInfo, "contactEmailType" ) == "Messenger3" )
    {
      contactInformations.insert( "isMessenger3", 1 );
      handle = XmlFunctions::getNodeValue( contactInfo, "email" ).toLower();
    }
    else
    {
      handle = XmlFunctions::getNodeValue( contactInfo, "passportName" ).toLower();
    }

    // The second condition is an HACK, the handle shouldn't be exist in the list of user
    // indeed is impossible to add it on the contact list because microsoft server doesn't accept
    // the request. The contact should be removed from the user's list, but for now we prefer to make
    // easy the life of the users...for the moment..:P
    if( handle.isEmpty() || handle == "messenger@microsoft.com" )
    {
      kWarning() << "Skipped 'messenger@microsoft.com' contact!";
      continue;
    }

    // Add the handle
    contactInformations.insert( "handle", handle );

    // Retrieve the other information about the contact
    // TODO implement the method for dynamic items,
    // please refer to msnpiki in the MSNP13 section.
    // and enable retrieval of the other services in
    // retrieveMembershipLists().

    // Add display name
    contactInformations.insert( "friendlyName", friendlyName );
    // Grep contact id
    information = XmlFunctions::getNodeValue( contact, "contactId" );
    contactInformations.insert( "contactId", information );
    // Determinate if the contact is messenger user or not
    information = XmlFunctions::getNodeValue( contactInfo, "isMessengerUser" );
    contactInformations.insert( "isMessengerUser", information );
    // Determinate if the contact has a space
    information = XmlFunctions::getNodeValue( contactInfo, "hasSpace" );
    contactInformations.insert( "hasSpace", information );

    // Check if the contact is assigned to any groups
    const QDomNodeList guids( contactInfo.toElement().elementsByTagName( "guid" ) );
    guidList.clear();
    if( ! guids.isEmpty() )
    {
      for( int i = 0; i < guids.count(); i++ )
      {
        guidList.append( guids.item( i ).toElement().text() );
      }

      contactInformations.insert( "guidList", guidList );
    }

    contacts.append( contactInformations );
  }

#ifdef KMESSDEBUG_ADDRESSBOOKSERVICE
  kDebug() << "Address book successfully parsed: found" << contacts.count() << "contacts and" << debugGroupCount << "groups.";
#endif

  // Signal that the of address book has been parsed
  emit gotAddressBookList( contacts );
}



// Parse the membership lists
void AddressBookService::parseMembershipLists( const QDomElement &body )
{
  const QDomNodeList services( body.elementsByTagName( "Service" ) );

  // New, empty accounts have no Services
  if( services.count() == 0 )
  {
    emit gotMembershipLists( "Messenger", QHash<QString,int>() );
    return;
  }

  for( int serviceIndex = 0; serviceIndex < services.count(); serviceIndex++ )
  {
    const QDomNode service( services.item( serviceIndex ) );

    // Get the name of the service
    QString serviceType( XmlFunctions::getNodeValue( service, "Info/Handle/Type" ) );

    if( serviceType.isEmpty() )
    {
      kWarning() << "Retrieved unknown service type!";
      continue;
    }

    const QDomNodeList memberships( XmlFunctions::getNode( service, "Memberships" ).childNodes() );

    // TODO Parse the timestamp of current retrieve to update the timestamp in the XML list
    /*
    QDomNodeList listInfo = body.elementsByTagName( "LastChange" );
    QString timestamp( listInfo.item( listInfo.count() - 1 ).toElement().text() );
    */

    int roleId;
    QString role;
    QString handle;

    // Parse the service's membership lists and save the results in the contactsRole hash table
    QHash<QString,int> contactsRole;

    for( int index = 0; index < memberships.count(); index++ )
    {
      // Parse each role structure
      const QDomNode membership( memberships.item( index ) );
      role = XmlFunctions::getNodeValue( membership, "MemberRole" );

      roleId = 0;
      if(      role == "Allow"   ) roleId = Contact::MSN_LIST_ALLOWED;
      else if( role == "Block"   ) roleId = Contact::MSN_LIST_BLOCKED;
      else if( role == "Reverse" ) roleId = Contact::MSN_LIST_REVERSE;
      else if( role == "Pending" ) roleId = Contact::MSN_LIST_PENDING;
      else
      {
        kWarning() << "Unknown membership role" << role << "in service" << serviceType << "!";
        continue;
      }

      // Parse each member
      const QDomNodeList members( membership.toElement().elementsByTagName( "Member" ) );
      for( int i = 0; i < members.count(); i++ )
      {
        // FIXME: This only works for the "Messenger" service. Others don't have the PassportName nor Email nodes.
        // We need a flexible system!
        const QDomNode memberNode( members.item( i ) );

        handle = XmlFunctions::getNodeValue( memberNode, "PassportName" ).toLower();

        if( handle.isEmpty() )
        {
          handle = XmlFunctions::getNodeValue( memberNode, "Email" ).toLower();

          if( handle.isEmpty() )
          {
            continue;
          }
        }
        // HACK: See parseAddressBook() for details
        else if( handle == "messenger@microsoft.com" )
        {
          continue;
        }

        // skip non-passport contacts in the membership lists (ie, Yahoo).
        // we can't handle them when manipulating the membership lists.
        QString type = XmlFunctions::getNodeValue( memberNode, "Type" ).toLower();
        if ( type != "passport" )
        {
          kDebug() << "Skipped non-passport contact" << handle;
          continue;
        }

        // Insert the current contact in the hash with their respective role
        // If the contact is already listed, the new value is OR'ed (set bit flag) with the old roleId
        contactsRole.insert( handle, contactsRole.value( handle, 0 ) | roleId );
      }
    }

    // When finished parsing the service's memberships list, signal that its list is ready
    emit gotMembershipLists( serviceType, contactsRole );
  }
}



/**
 * @brief  Parse a SOAP error message.
 *
 * This method interprets incoming fault messages from the server.
 *
 * @param  message   The message containing a soap fault conXML node which contains the SOAP fault.
 */
void AddressBookService::parseSecureFault( SoapMessage *message )
{
  // Should never happen
  if( ! message->isFaultMessage() )
  {
    kWarning() << "Incorrect fault detected on incoming message";
    return;
  }

  // Get the type of error
  QString faultCode  ( message->getFaultCode()        );
  QString faultString( message->getFaultDescription() );
  QString errorCode  ( XmlFunctions::getNodeValue( message->getFault(), "detail/errorcode" ) );

  // Get the message data
  MessageData messageData( message->getData() );
  QString     type       ( messageData.type   );
  QVariant    info       ( messageData.value  );

  // See which fault we received
  if( faultCode == "soap:Client" && ! errorCode.isEmpty() )
  {
    // We forgot to check if the email was valid:
    if( errorCode == "BadEmailArgument" )
    {
      QString email( info.toStringList().first() );

      kWarning() << "Malformed email address found by the server:" << email;

      emit soapWarning( i18nc( "Warning message",
                               "The specified email address, \"%1\", is not a valid email address!",
                               email ),
                        true /* isImportant */ );
    }
    // The email we tried to add is not an address registered on Passport
    else if( errorCode == "InvalidPassportUser" )
    {
      QString email( info.toStringList().first() );

      emit soapWarning( i18nc( "Error message",
                               "The specified email address, \"%1\", does not belong to a Live Messenger account!",
                               email ),
                        true /* isImportant */ );
    }
    // We messed up something in the header. Our bad, sorry!
    else if( errorCode == "InvalidApplicationHeader" )
    {
      emit soapError( i18nc( "Error message (system-generated description)",
                             "Invalid web service request (%1)", message->getFaultDescription() ),
                      MsnSocketBase::ERROR_INTERNAL );
    }
    // This is a new account which doesn't have an address book. Create one
    else if( errorCode == "ABDoesNotExist" )
    {
#ifdef KMESSDEBUG_ADDRESSBOOKSERVICE
      kDebug() << "Address book does not exist: creating new one.";
#endif
      createAddressBook();
      return;
    }
#ifdef KMESSDEBUG_HTTPSOAPCONNECTION
    else
    {
      // Do nothing, the HttpSoapConnection debug output should suffice
    }
#endif
  }
  // There was another unspecified error with our SOAP request:
  else
  {
    emit soapError( i18nc( "Error message (system-generated description)",
                          "Invalid web service request (%1)", message->getFaultDescription() ),
                    MsnSocketBase::ERROR_INTERNAL );
  }
}



/**
 * @brief Parse the result of the response from the server
 *
 * We can parse two different soap replies:
 * 1) the membership lists that contains the lists of contacts and them
 *    respective roles: Allow, Block, Reverse, Pending.
 * 2) the address book list that contains the groups and the contacts with them friendlyname, id etc..
 *
 * @param message the SOAP response message.
 * @see PassportLoginService::parseSoapResult().
 */
void AddressBookService::parseSecureResult( SoapMessage *message )
{
  QDomElement body( message->getBody().toElement() );
  QString     resultName( body.localName() );

  if( ! body.firstChildElement( "FindMembershipResponse" ).isNull() )
  {
    parseMembershipLists( body );
    return;
  }
  else if( ! body.firstChildElement( "ABFindAllResponse" ).isNull() )
  {
    parseAddressBook( body );
    return;
  }
  else if( ! body.firstChildElement( "ABContactUpdateResponse" ).isNull() )
  {
    // Updated a contact's info
    return;
  }
  else if( ! body.firstChildElement( "DeleteMemberResponse" ).isNull() )
  {
    // Contact removed from Membership list from selected Role
    return;
  }
  else if( ! body.firstChildElement( "ABAddResponse" ).isNull() )
  {
    // New address book was created: proceed with it
#ifdef KMESSDEBUG_ADDRESSBOOKSERVICE
    kDebug() << "Address book successfully created: retrying to obtain membership lists.";
#endif
    retrieveMembershipLists();
    return;
  }

  // Get the message's data
  MessageData data( message->getData() );

  if( data.type == "ContactAdd" )
  {
    // A contact was added to the AB

          QStringList  info     ( data.value.toStringList() );
    const QString      handle   ( info.takeFirst() );
    const QStringList  groupsId ( info );
    const QString      contactId( body.elementsByTagName( "guid" ).item(0).toElement().text() );

    // If the contact is valid, signal that the contact was added to AB
    if( ! contactId.isEmpty() && ! handle.isEmpty() )
    {
      emit contactAdded( handle, contactId, groupsId );
    }
  }
  else if( data.type == "ContactDelete" )
  {
    // A contact was deleted from the AB

    const QString contactId( data.value.toString() );

    if( ! contactId.isEmpty() )
    {
      emit contactDeleted( contactId );
    }
  }
  else if( data.type == "ContactBlock" || data.type == "ContactUnblock" )
  {
    // A contact was added to the Membership list in a certain Role

    const QStringList info  ( data.value.toStringList() );
    const QString     handle( info.value( 0 ) );
    const QString     role  ( info.value( 1 ) );

    if( role == "BL" )
    {
      emit contactBlocked( handle );
    }
    else if( role == "AL" )
    {
      emit contactUnblocked( handle );
    }
  }
  else if( data.type == "ContactAddToGroup" )
  {
    // A contact was added to a group

    const QString groupId  ( data.value.toString() );
    const QString contactId( body.elementsByTagName( "guid" ).item(0).toElement().text() );

    if( ! contactId.isEmpty() && ! groupId.isEmpty() )
    {
      emit contactAddedToGroup( contactId, groupId );
    }
  }
  else if( data.type == "ContactDeleteFromGroup" )
  {
    // A contact was removed from a group

    const QStringList info     ( data.value.toStringList() );
    const QString     contactId( info.value( 0 ) );
    const QString     groupId  ( info.value( 1 ) );

    if( ! contactId.isEmpty() && ! groupId.isEmpty() )
    {
      emit contactDeletedFromGroup( contactId, groupId );
    }
  }
  else if( data.type == "GroupAdd" )
  {
    // A group was added
    const QString name   ( data.value.toString() );
    const QString groupId( body.elementsByTagName( "guid" ).item(0).toElement().text() );

    if( ! groupId.isEmpty() && ! name.isEmpty() )
    {
      emit groupAdded( groupId, name );
    }
  }
  else if( data.type == "GroupDelete" )
  {
    // A group was removed
    const QString groupId( data.value.toString() );

    if( ! groupId.isEmpty() )
    {
      emit groupDeleted( groupId );
    }
  }
  else if( data.type == "GroupRename" )
  {
    // A group was renamed
    const QStringList info   ( data.value.toStringList() );
    const QString     groupId( info.value( 0 ) );
    const QString     name   ( info.value( 1 ) );

    if( ! groupId.isEmpty() && ! name.isEmpty() )
    {
      emit groupRenamed( groupId, name );
    }
  }
  else
  {
    kWarning() << "Response data not recognized:" << data.type;
  }
}



// Unblock contact
void AddressBookService::unblockContact( const QString &handle )
{
  SoapMessage *message = getMembershipListUpdate( handle, "Allow", true );

  MessageData data;
  data.type  = "ContactUnblock";
  data.value = QStringList() << handle << "AL";

  message->setData( data );

  sendSecureRequest( message, "Contacts" );
}



// Create a message to update the membership list
SoapMessage *AddressBookService::getMembershipListUpdate( const QString &handle, const QString &role, bool adding )
{
  QString body( "xmlns=\"http://www.msn.com/webservices/AddressBook\">\n"
                "  <serviceHandle>\n"
                "    <Id>0</Id>\n"
                "    <Type>Messenger</Type>\n"
                "    <ForeignId></ForeignId>\n"
                "  </serviceHandle>\n"
                "  <memberships>\n"
                "    <Membership>\n"
                "      <MemberRole>" + role + "</MemberRole>\n"
                "      <Members>\n"
                "        <Member xsi:type=\"PassportMember\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n"
                "          <Type>Passport</Type>\n"
                "          <State>Accepted</State>\n"
                "          <PassportName>" + handle + "</PassportName>\n"
                "        </Member>\n"
                "      </Members>\n"
                "    </Membership>\n"
                "  </memberships>\n" );

  QString action;
  if( adding )
  {
    action = "http://www.msn.com/webservices/AddressBook/AddMember";
    body = "<AddMember " + body + "</AddMember>";
  }
  else
  {
    action = "http://www.msn.com/webservices/AddressBook/DeleteMember";
    body = "<DeleteMember " + body + "</DeleteMember>";
  }

  return new SoapMessage( SERVICE_URL_ADDRESSBOOK_SHARING,
                          action,
                          createCommonHeader( "BlockUnblock" ),
                          body );
}



#include "addressbookservice.moc"
