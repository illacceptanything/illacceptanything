<?php

if (!function_exists('hash_equals')) {
    /**
     * An implementation for the `hash_equals()` function.
     *
     * This exists to support PHP versions prior to 5.6 and is meant to work the
     * same as PHP's function. The original code was written by Rouven Weßling.
     *
     * @param string $knownString Calculated hash.
     * @param string $userString  User-provided hash.
     *
     * @return bool
     * @copyright Copyright (c) 2013-2014 Rouven Weßling <http://rouvenwessling.de>
     * @license http://opensource.org/licenses/MIT MIT
     */
    function hash_equals($knownString, $userString)
    {
        $argc = func_num_args();
        if ($argc < 2) {
            trigger_error(
                "hash_equals() expects at least 2 parameters, {$argc} given",
                E_USER_WARNING
            );

            return null;
        }

        if (!is_string($knownString)) {
            trigger_error(sprintf(
                "hash_equals(): Expected known_string to be a string, %s given",
                gettype($knownString)
            ), E_USER_WARNING);

            return false;
        }

        if (!is_string($userString)) {
            trigger_error(sprintf(
                "hash_equals(): Expected user_string to be a string, %s given",
                gettype($knownString)
            ), E_USER_WARNING);

            return false;
        }

        if (strlen($knownString) !== strlen($userString)) {
            return false;
        }

        $len = strlen($knownString);
        $result = 0;
        for ($i = 0; $i < $len; $i++) {
            $result |= (ord($knownString[$i]) ^ ord($userString[$i]));
        }

        return 0 === $result;
    }
}
