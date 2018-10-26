/***************************************************************************
                          addressbookservice.h
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

#ifndef ADDRESSBOOK_H
#define ADDRESSBOOK_H

#include "passportloginservice.h"

#include <QHash>
#include <QStringList>


/**
 * @brief Soap actions for retrieve the address book and membership lists.
 *
 *
 * @author Antonio Nastasi
 * @ingroup NetworkSoap
 */
class AddressBookService : public PassportLoginService
{
  Q_OBJECT

  public:
    enum ContactProperty
    {
      PROPERTY_FRIENDLYNAME = 0,
      PROPERTY_PRIVACY
    };

    // The constructor
                        AddressBookService( QObject *parent = 0 );
    // The destructor
    virtual            ~AddressBookService();
    // Add contact to AB
    void                addContact( const QString &handle, const QStringList& groupsId = QStringList(), bool alreadyExists = false );
    // Add contact to group
    void                addContactToGroup( const QString &contactId, const QString &groupId );
    // Add group
    void                addGroup( const QString &name );
    // Block contact
    void                blockContact( const QString &handle );
    // Put the Soap action to update the information about contact
    void                contactUpdate( ContactProperty property, const QString &newValue );
    // Delete contact
    void                deleteContact( const QString &contactId );
    // Delete contact from group
    void                deleteContactFromGroup( const QString &contactId, const QString &groupId );
    // Delete group
    void                deleteGroup( const QString &groupId );
    // Create a message to update the membership list
    SoapMessage        *getMembershipListUpdate( const QString &handle, const QString &role, bool adding );
    // Mark a contact as a Messenger user
    void                markMessengerUser( const QString &contactId );
    // Rename group
    void                renameGroup( const QString &groupId, const QString &name );
    // Remove a user from the PL
    void                removePending( const QString &handle );
    // Retrieve the address book
    void                retrieveAddressBook( const QString &fromTimestamp = QString() );
    // Retrieve the membership lists
    void                retrieveMembershipLists();
    // Retrieve the Dynamic Items: Gleams
    void                retrieveGleams();
    // Unblock contact
    void                unblockContact( const QString &handle );

  private: // private methods
    // Request creation of a new address book (for new accounts)
    void                createAddressBook();
    // Create the common header for the soap requests
    QString             createCommonHeader( const QString partnerScenario = "Initial" );
    // Parse the address book
    void                parseAddressBook( const QDomElement &body );
    // Parse the membership lists
    void                parseMembershipLists( const QDomElement &body );
    // Parse a SOAP error message.
    void                parseSecureFault( SoapMessage *message );
    // Parse the result of the response from the server
    void                parseSecureResult( SoapMessage *message );

  signals: // Contact Address Book signals
    // Contact was added
    void                contactAdded( const QString &handle, const QString &contactId, const QStringList &groupsId );
    // Contact was added to group
    void                contactAddedToGroup( const QString &contactId, const QString &groupId );
    // Contact was blocked
    void                contactBlocked( const QString &handle );
    // Contact was deleted
    void                contactDeleted( const QString &contactId );
    // Contact was delete from group
    void                contactDeletedFromGroup( const QString &contactId, const QString &groupId );
    // Contact was unblocked
    void                contactUnblocked( const QString &handle );

  signals: // Group Address Book signals
    // Group was added
    void                groupAdded( const QString& groupId, const QString& name );
    // Group was removed
    void                groupDeleted( const QString& groupId );
    // Group was renamed
    void                groupRenamed( const QString& groupId, const QString& name );
    // A group's details have been received
    void                gotGroup( const QString &guid, const QString &name );

  signals: // Generic Address Book signals
    // Received a membership list, with its contacts and their respective roles
    void                gotMembershipLists( const QString &serviceType, const QHash<QString,int> &contactsRole );
    // Received information about the logged in contact
    void                gotPersonalInformation( const QString &cid, int blp );
    // The address book was entirely received and parsed
    void                gotAddressBookList( const QList< QHash<QString,QVariant> > &contacts );
};

#endif
