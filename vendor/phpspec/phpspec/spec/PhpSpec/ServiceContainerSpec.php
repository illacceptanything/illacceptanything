<?php

namespace spec\PhpSpec;

use PhpSpec\ObjectBehavior;

class ServiceContainerSpec extends ObjectBehavior
{
    function it_stores_parameters()
    {
        $this->setParam('some_param', 42);
        $this->getParam('some_param')->shouldReturn(42);
    }

    function it_returns_null_value_for_unexisting_parameter()
    {
        $this->getParam('unexisting')->shouldReturn(null);
    }

    function it_returns_custom_default_for_unexisting_parameter_if_provided()
    {
        $this->getParam('unexisting', 42)->shouldReturn(42);
    }

    function it_stores_services($service)
    {
        $this->set('some_service', $service);
        $this->get('some_service')->shouldReturn($service);
    }

    function it_throws_exception_when_trying_to_get_unexisting_service()
    {
        $this->shouldThrow('InvalidArgumentException')->duringGet('unexisting');
    }

    function it_evaluates_factory_function_set_as_service()
    {
        $this->set('random_number', function () { return rand(); });
        $number1 = $this->get('random_number');
        $number2 = $this->get('random_number');

        $number1->shouldBeInteger();
        $number2->shouldBeInteger();

        $number2->shouldNotBe($number1);
    }

    function it_evaluates_factory_function_only_once_for_shared_services()
    {
        $this->setShared('random_number', function () { return rand(); });
        $number1 = $this->get('random_number');
        $number2 = $this->get('random_number');

        $number2->shouldBe($number1);
    }

    function it_provides_a_way_to_retrieve_services_by_prefix($service1, $service2, $service3)
    {
        $this->set('collection1.serv1', $service1);
        $this->set('collection1.serv2', $service2);
        $this->set('collection2.serv3', $service3);

        $this->getByPrefix('collection1')->shouldReturn(array($service1, $service2));
    }

    function it_provides_a_way_to_remove_service_by_key($service)
    {
        $this->set('collection1.some_service', $service);
        $this->remove('collection1.some_service');

        $this->shouldThrow()->duringGet('collection1.some_service');
        $this->getByPrefix('collection1')->shouldHaveCount(0);
    }

    function it_supports_custom_service_configurators()
    {
        $this->addConfigurator(function ($c) {
            $c->setParam('name', 'Jim');
        });
        $this->configure();

        $this->getParam('name')->shouldReturn('Jim');
    }
}
