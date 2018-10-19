<?php

class GetFilteredDataSetResult
{

    /**
     * @var string $schema
     * @access public
     */
    public $schema = null;

    /**
     * @var string $any
     * @access public
     */
    public $any = null;

    /**
     * @param string $schema
     * @param string $any
     * @access public
     */
    public function __construct($schema, $any)
    {
      $this->schema = $schema;
      $this->any = $any;
    }

}
