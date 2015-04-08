<?php

/*
 * This file is part of the Symfony package.
 *
 * (c) Fabien Potencier <fabien@symfony.com>
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

namespace Symfony\Component\Security\Core\Tests\Validator\Constraints;

use Symfony\Component\Validator\Validation;

/**
 * @since  2.5.4
 * @author Bernhard Schussek <bschussek@gmail.com>
 */
class LegacyUserPasswordValidatorApiTest extends UserPasswordValidatorTest
{
    protected function getApiVersion()
    {
        return Validation::API_VERSION_2_5_BC;
    }
}
