<?php

class GetFilteredDataSetResponse
{

    /**
     * @var GetFilteredDataSetResult $GetFilteredDataSetResult
     * @access public
     */
    public $GetFilteredDataSetResult = null;

    /**
     * @param GetFilteredDataSetResult $GetFilteredDataSetResult
     * @access public
     */
    public function __construct($GetFilteredDataSetResult)
    {
      $this->GetFilteredDataSetResult = $GetFilteredDataSetResult;
    }

}
