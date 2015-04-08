<?php
class PoopException extends Exception {}

try {
    echo "fart!\n";
    throw new PoopException('oops!');
} catch (PoopException $e) {
    echo "Wiped exception: ",  $e->getMessage(), "\n";
}
?>
