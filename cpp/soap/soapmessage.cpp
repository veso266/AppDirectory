/***************************************************************************
                          soapmessage.cpp  -  description
                             -------------------
    begin                : Sun Jul 24 2005
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

#include "soapmessage.h"

#include "../../utils/xmlfunctions.h"
#include "../../kmessdebug.h"

#include <QStringList>


#ifdef KMESSDEBUG_SOAPMESSAGE
  #define KMESSDEBUG_SOAPMESSAGE_GENERAL
#endif


#define XMLNS_XSD  "http://www.w3.org/2001/XMLSchema"
#define XMLNS_XSI  "http://www.w3.org/2001/XMLSchema-instance"
#define XMLNS_SOAP "http://schemas.xmlsoap.org/soap/envelope/"



// The constructor
SoapMessage::SoapMessage( const QString &endPointUrl, const QString &action, const QString &header, const QString &body, const MessageData &data )
: action_( action )
, data_( data )
, endPoint_( endPointUrl )
{
  // Assemble an essential envelope for the parser
  QString envelope( "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                    "<soap:Envelope xmlns:xsi=\""  XMLNS_XSI  "\""
                                  " xmlns:xsd=\""  XMLNS_XSD  "\""
                                  " xmlns:soap=\"" XMLNS_SOAP "\">\n" );

  if( ! header.isEmpty() )
  {
    envelope += "<soap:Header>" + header + "</soap:Header>";
  }

  if( ! body.isEmpty() )
  {
    envelope += "<soap:Body>" + body + "</soap:Body>";
  }

  envelope += "</soap:Envelope>";

  setMessage( envelope );
}



// The copy constructor
SoapMessage::SoapMessage( const SoapMessage &other )
: action_( other.action_ )
, body_( other.body_ )
, data_( other.data_ )
, endPoint_( other.endPoint_ )
, fault_( other.fault_ )
, faultCode_( other.faultCode_ )
, faultDescription_( other.faultDescription_ )
, isValid_( other.isValid_ )
, header_( other.header_ )
{
}



// The destructor
SoapMessage::~SoapMessage()
{
}



// Return the message action
const QString& SoapMessage::getAction() const
{
  return action_;
}



// Return the associated request data
const MessageData& SoapMessage::getData() const
{
  return data_;
}



// Return the URL of the SOAP endpoint to which the message will be sent
const QString& SoapMessage::getEndPoint() const
{
  return endPoint_;
}



// Return the message fault's xml tree
const QDomNode& SoapMessage::getFault() const
{
  return fault_;
}



// Return the error code
const QString& SoapMessage::getFaultCode() const
{
  return faultCode_;
}



// Return the error description
const QString &SoapMessage::getFaultDescription() const
{
  return faultDescription_;
}



// Return the entire message as a byte array
QByteArray SoapMessage::getMessage() const
{
  // Just do nothing if there's no data
  if( ! isValid_ )
  {
    return QByteArray();
  }

  QByteArray contents;
  QTextStream stream( &contents, QIODevice::Append );

  // Write the header
  if( ! header_.isNull() )
  {
    stream << header_ << "\n";
//     stream << "<soap:Header>\n" << header_ << "</soap:Header>\n";
  }

  // Write the body
  if( ! body_.isNull() )
  {
    stream << body_ << "\n";
//     stream << "<soap:Body>\n" << body_ << "</soap:Body>\n";
  }

  // Other blocks are not included, since this method is used for common use, not corner cases

  // Return the complete data message
  return "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
         "<soap:Envelope xmlns:xsi=\""  XMLNS_XSI  "\""
                       " xmlns:xsd=\""  XMLNS_XSD  "\""
                       " xmlns:soap=\"" XMLNS_SOAP "\">\n"
         + contents +
         "</soap:Envelope>\n";
}



// Return the message body's xml tree
const QDomNode& SoapMessage::getBody() const
{
  return body_;
}



// Return the message header's xml tree
QDomNode& SoapMessage::getHeader()
{
  return header_;
}



// Return whether this was an error response. Note that you should only use this method on valid messages.
bool SoapMessage::isFaultMessage() const
{
  return ( ! faultCode_.isEmpty() );
}



// Return whether this message contains valid useable data
bool SoapMessage::isValid() const
{
  return isValid_;
}



// Change the associated request data
void SoapMessage::setData( const MessageData &data )
{
  data_ = data;
}



// Parse the incoming message
void SoapMessage::setMessage( const QString &message )
{
  QDomDocument xml;

  // Reset the error indicator
  faultCode_ = QString();

  // Parse the XML
  // see http://doc.trolltech.com/4.3/qdomdocument.html#setContent
  isValid_ = xml.setContent( message, true, &faultDescription_ ); // true for namespace processing.

  // Create an error message if the message can't be parsed
  if( ! isValid_ )
  {
    faultCode_ = "parsingFailed";
    body_      = QDomNode();
    fault_     = QDomNode();
    header_    = QDomNode();

    kWarning() << "XML parsing failed (code" << faultCode_ << "):" << faultDescription_ << ".";
    return;
  }

  // Get the message's child nodes
  header_    = XmlFunctions::getNode( xml, "/Envelope/Header" );
  body_      = XmlFunctions::getNode( xml, "/Envelope/Body" );

  // Verify if any faults are present in the message
  QDomNode rootFault = XmlFunctions::getNode( xml, "/Envelope/Fault" );
  QDomNode bodyFault;
  // Only check body_ if it's not null, or the assert in getNode will fail
  if( ! body_.isNull() )
  {
    bodyFault = XmlFunctions::getNode( body_, "/Fault" );
  }

  if( ! rootFault.isNull() )
  {
    fault_            = rootFault;
    faultCode_        = XmlFunctions::getNodeValue( rootFault, "/faultcode"   );
    faultDescription_ = XmlFunctions::getNodeValue( rootFault, "/faultstring" );
  }
  else if( ! bodyFault.isNull() )
  {
    fault_            = bodyFault;
    faultCode_        = XmlFunctions::getNodeValue( bodyFault, "/faultcode"   );
    faultDescription_ = XmlFunctions::getNodeValue( bodyFault, "/faultstring" );
  }

  // Catch empty messages... unlikely
  if( fault_.isNull() && header_.isNull() && body_.isNull() )
  {
    isValid_          = false;
    faultCode_        = "missingBody";
    faultDescription_ = "message body is not present";

    kWarning() << "XML parsing failed (code" << faultCode_ << "):" << faultDescription_ << ".";
    return;
  }
}



