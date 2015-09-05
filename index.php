<?php

/**
 * Created by PhpStorm.
 * User: seyfer
 * Date: 05.09.15
 * Time: 18:31
 */
class PoopException extends Exception
{
}

try {
    echo "fart!\n";
    throw new PoopException('oops!');
} catch (PoopException $e) {
    echo "Wiped exception: ", $e->getMessage(), "\n";
}
