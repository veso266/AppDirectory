<?php

class GetAppEntryResponse
{

    /**
     * @var Entry $GetAppEntryResult
     * @access public
     */
    public $GetAppEntryResult = null;

    /**
     * @param Entry $GetAppEntryResult
     * @access public
     */
    public function __construct($GetAppEntryResult)
    {
      $this->GetAppEntryResult = $GetAppEntryResult;
    }

}
