<?php

include_once('GetAppEntry.php');
include_once('GetAppEntryResponse.php');
include_once('Entry.php');
include_once('GetFullDataSet.php');
include_once('GetFullDataSetResponse.php');
include_once('GetFullDataSetResult.php');
include_once('GetFilteredDataSet.php');
include_once('GetFilteredDataSetResponse.php');
include_once('GetFilteredDataSetResult.php');


/**
 * Returns data about individual P4 apps or P4 app categories
 */
class AppDirectory extends \SoapClient
{

    /**
     * @var array $classmap The defined classes
     * @access private
     */
    private static $classmap = array(
      'GetAppEntry' => '\GetAppEntry',
      'GetAppEntryResponse' => '\GetAppEntryResponse',
      'Entry' => '\Entry',
      'GetFullDataSet' => '\GetFullDataSet',
      'GetFullDataSetResponse' => '\GetFullDataSetResponse',
      'GetFullDataSetResult' => '\GetFullDataSetResult',
      'GetFilteredDataSet' => '\GetFilteredDataSet',
      'GetFilteredDataSetResponse' => '\GetFilteredDataSetResponse',
      'GetFilteredDataSetResult' => '\GetFilteredDataSetResult');

    /**
     * @param array $options A array of config values
     * @param string $wsdl The wsdl file to use
     * @access public
     */
    public function __construct(array $options = array(), $wsdl = 'AppDirectory.xml')
    {
      foreach (self::$classmap as $key => $value) {
    if (!isset($options['classmap'][$key])) {
      $options['classmap'][$key] = $value;
    }
  }
  
  parent::__construct($wsdl, $options);
    }

    /**
     * @param GetAppEntry $parameters
     * @access public
     * @return GetAppEntryResponse
     */
    public function GetAppEntry(GetAppEntry $parameters)
    {
      return $this->__soapCall('GetAppEntry', array($parameters));
    }

    /**
     * @param GetFullDataSet $parameters
     * @access public
     * @return GetFullDataSetResponse
     */
    public function GetFullDataSet(GetFullDataSet $parameters)
    {
      return $this->__soapCall('GetFullDataSet', array($parameters));
    }

    /**
     * @param GetFilteredDataSet $parameters
     * @access public
     * @return GetFilteredDataSetResponse
     */
    public function GetFilteredDataSet(GetFilteredDataSet $parameters)
    {
      return $this->__soapCall('GetFilteredDataSet', array($parameters));
    }

}
