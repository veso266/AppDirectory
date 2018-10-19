<?php

class GetAppEntry
{

    /**
     * @var int $ID
     * @access public
     */
    public $ID = null;

    /**
     * @param int $ID
     * @access public
     */
    public function __construct($ID)
    {
      $this->ID = $ID;
    }

}
