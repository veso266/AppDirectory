<?php 
class AppDirectory
{
    /**
     * Say hello.
     *
     * @param string $firstName
     * @return string $greetings
     */
	 
    public function sayHello($firstName)
    {
        return 'Hello ' . $firstName;
    }
	
	/**
     * Gets Filtered data obout P3, P4 Apps.
     *
     * @param string $locale
     * @param int $Page
     * @param int $Kids
     * @param int $AppType
	 * @access public
     * @return array $test
     */
	public function GetFilteredDataSet2($locale, $Page, $Kids, $AppType) 
	{
		//$test = file_get_contents('../test.xml');
		@$my_xml = 
		"
<Entry>
  <EntryID>1</EntryID>
  <Error>Error</Error>
  <Locale>en-us</Locale>
  <Kids>1</Kids>
  <Page>1</Page>
  <Category>50</Category>
  <Sequence>10</Sequence>
  <Name>File Sharing</Name>
  <Description>Share Files Between Two Users!</Description>
  <URL>http://dxing.si/msn/AppDirectory/P4Apps/FileSharing/en/fileSharingCtrl.htm</URL>
  <IconURL>http://example.org/example.png</IconURL>
  <PassportSiteID>0</PassportSiteID>
  <Type>Dir</Type>
  <Height>500</Height>
  <Width>500</Width>
  <Location>side</Location>
  <MinUsers>2</MinUsers>
  <MaxUsers>2</MaxUsers>
  <PassportSiteID>0</PassportSiteID>
  <EnableIP>False</EnableIP>
  <ActiveX>True</ActiveX>
  <SendFile>True</SendFile>
  <SendIM>False</SendIM>
  <ReceiveIM>False</ReceiveIM>
  <ReplaceIM>False</ReplaceIM>
  <Windows>False</Windows>
  <MaxPacketRate>120</MaxPacketRate>
  <UserProperties>False</UserProperties>  
  <ClientVersion>7.5</ClientVersion>
  <AppType>0</AppType>  
  <Hidden>false</Hidden>
</Entry>
		";	
		//return new SoapVar('<ns1:xmlDocument>'.$my_xml.'</ns1:xmlDocument>',XSD_ANYXML);
		//return new SoapVar($my_xml ,XSD_ANYXML);
		return new SoapVar('<GetFilteredDataSet2Result>'.$my_xml.'</GetFilteredDataSet2Result>' ,XSD_ANYXML);
	}
	
}
?>