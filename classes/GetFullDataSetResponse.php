<?php

class GetFullDataSetResponse
{

    /**
     * @var GetFullDataSetResult $GetFullDataSetResult
     * @access public
     */
    public $GetFullDataSetResult = null;

    /**
     * @param GetFullDataSetResult $GetFullDataSetResult
     * @access public
     */
    public function __construct($GetFullDataSetResult)
    {
      $this->GetFullDataSetResult = $GetFullDataSetResult;
    }

}
