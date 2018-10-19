<?php

class Entry
{

    /**
     * @var int $EntryID
     * @access public
     */
    public $EntryID = null;

    /**
     * @var string $Error
     * @access public
     */
    public $Error = null;

    /**
     * @var string $Locale
     * @access public
     */
    public $Locale = null;

    /**
     * @var int $Kids
     * @access public
     */
    public $Kids = null;

    /**
     * @var int $Page
     * @access public
     */
    public $Page = null;

    /**
     * @var int $CategoryID
     * @access public
     */
    public $CategoryID = null;

    /**
     * @var int $Sequence
     * @access public
     */
    public $Sequence = null;

    /**
     * @var string $Name
     * @access public
     */
    public $Name = null;

    /**
     * @var string $Description
     * @access public
     */
    public $Description = null;

    /**
     * @var string $URL
     * @access public
     */
    public $URL = null;

    /**
     * @var string $IconURL
     * @access public
     */
    public $IconURL = null;

    /**
     * @var string $AppIconURL
     * @access public
     */
    public $AppIconURL = null;

    /**
     * @var string $Type
     * @access public
     */
    public $Type = null;

    /**
     * @var int $Height
     * @access public
     */
    public $Height = null;

    /**
     * @var int $Width
     * @access public
     */
    public $Width = null;

    /**
     * @var string $Location
     * @access public
     */
    public $Location = null;

    /**
     * @var int $MinUsers
     * @access public
     */
    public $MinUsers = null;

    /**
     * @var int $MaxUsers
     * @access public
     */
    public $MaxUsers = null;

    /**
     * @var int $PassportSiteID
     * @access public
     */
    public $PassportSiteID = null;

    /**
     * @var boolean $EnableIP
     * @access public
     */
    public $EnableIP = null;

    /**
     * @var boolean $ActiveX
     * @access public
     */
    public $ActiveX = null;

    /**
     * @var boolean $SendFile
     * @access public
     */
    public $SendFile = null;

    /**
     * @var boolean $SendIM
     * @access public
     */
    public $SendIM = null;

    /**
     * @var boolean $ReceiveIM
     * @access public
     */
    public $ReceiveIM = null;

    /**
     * @var boolean $ReplaceIM
     * @access public
     */
    public $ReplaceIM = null;

    /**
     * @var boolean $Windows
     * @access public
     */
    public $Windows = null;

    /**
     * @var int $MaxPacketRate
     * @access public
     */
    public $MaxPacketRate = null;

    /**
     * @var string $LogURL
     * @access public
     */
    public $LogURL = null;

    /**
     * @var boolean $UserProperties
     * @access public
     */
    public $UserProperties = null;

    /**
     * @var string $SubscriptionURL
     * @access public
     */
    public $SubscriptionURL = null;

    /**
     * @var string $ClientVersion
     * @access public
     */
    public $ClientVersion = null;

    /**
     * @var int $AppType
     * @access public
     */
    public $AppType = null;

    /**
     * @var boolean $Hidden
     * @access public
     */
    public $Hidden = null;

    /**
     * @param int $EntryID
     * @param string $Error
     * @param string $Locale
     * @param int $Kids
     * @param int $Page
     * @param int $CategoryID
     * @param int $Sequence
     * @param string $Name
     * @param string $Description
     * @param string $URL
     * @param string $IconURL
     * @param string $AppIconURL
     * @param string $Type
     * @param int $Height
     * @param int $Width
     * @param string $Location
     * @param int $MinUsers
     * @param int $MaxUsers
     * @param int $PassportSiteID
     * @param boolean $EnableIP
     * @param boolean $ActiveX
     * @param boolean $SendFile
     * @param boolean $SendIM
     * @param boolean $ReceiveIM
     * @param boolean $ReplaceIM
     * @param boolean $Windows
     * @param int $MaxPacketRate
     * @param string $LogURL
     * @param boolean $UserProperties
     * @param string $SubscriptionURL
     * @param string $ClientVersion
     * @param int $AppType
     * @param boolean $Hidden
     * @access public
     */
    public function __construct($EntryID, $Error, $Locale, $Kids, $Page, $CategoryID, $Sequence, $Name, $Description, $URL, $IconURL, $AppIconURL, $Type, $Height, $Width, $Location, $MinUsers, $MaxUsers, $PassportSiteID, $EnableIP, $ActiveX, $SendFile, $SendIM, $ReceiveIM, $ReplaceIM, $Windows, $MaxPacketRate, $LogURL, $UserProperties, $SubscriptionURL, $ClientVersion, $AppType, $Hidden)
    {
      $this->EntryID = $EntryID;
      $this->Error = $Error;
      $this->Locale = $Locale;
      $this->Kids = $Kids;
      $this->Page = $Page;
      $this->CategoryID = $CategoryID;
      $this->Sequence = $Sequence;
      $this->Name = $Name;
      $this->Description = $Description;
      $this->URL = $URL;
      $this->IconURL = $IconURL;
      $this->AppIconURL = $AppIconURL;
      $this->Type = $Type;
      $this->Height = $Height;
      $this->Width = $Width;
      $this->Location = $Location;
      $this->MinUsers = $MinUsers;
      $this->MaxUsers = $MaxUsers;
      $this->PassportSiteID = $PassportSiteID;
      $this->EnableIP = $EnableIP;
      $this->ActiveX = $ActiveX;
      $this->SendFile = $SendFile;
      $this->SendIM = $SendIM;
      $this->ReceiveIM = $ReceiveIM;
      $this->ReplaceIM = $ReplaceIM;
      $this->Windows = $Windows;
      $this->MaxPacketRate = $MaxPacketRate;
      $this->LogURL = $LogURL;
      $this->UserProperties = $UserProperties;
      $this->SubscriptionURL = $SubscriptionURL;
      $this->ClientVersion = $ClientVersion;
      $this->AppType = $AppType;
      $this->Hidden = $Hidden;
    }

}
