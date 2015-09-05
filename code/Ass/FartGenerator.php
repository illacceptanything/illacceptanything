<?php
/**
 * Created by PhpStorm.
 * User: seyfer
 * Date: 05.09.15
 * Time: 18:36
 */

namespace Ass;

/**
 * Class FartGenerator
 * @package Ass
 */
class FartGenerator
{
    /**
     * @throws PoopException
     */
    public static function generate()
    {
        try {
            $fart = new ConcreteFart();

            echo "$fart\n";

            throw new PoopException('oops!');
        } catch (PoopException $e) {
            echo "Wiped exception: ", $e->getMessage(), "\n";
        }
    }
}