<?php

class GetFilteredDataSet
{

    /**
     * @var int $LCID
     * @access public
     */
    public $LCID = null;

    /**
     * @var int $Page
     * @access public
     */
    public $Page = null;

    /**
     * @var int $Kids
     * @access public
     */
    public $Kids = null;

    /**
     * @var int $AppType
     * @access public
     */
    public $AppType = null;

    /**
     * @param int $LCID
     * @param int $Page
     * @param int $Kids
     * @param int $AppType
     * @access public
     */
    public function __construct($LCID, $Page, $Kids, $AppType)
    {
      $this->LCID = $LCID;
      $this->Page = $Page;
      $this->Kids = $Kids;
      $this->AppType = $AppType;
    }

}
