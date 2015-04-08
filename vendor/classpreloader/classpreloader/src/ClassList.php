<?php

namespace ClassPreloader;

/**
 * This is the class list class.
 *
 * This maintains a list of classes using a sort of doubly-linked list.
 */
class ClassList
{
    /**
     * The head node of the list.
     *
     * @var \ClassPreloader\ClassNode
     */
    protected $head;

    /**
     * The current node of the list.
     *
     * @var \ClassPreloader\ClassNode
     */
    protected $current;

    /**
     * Create a new class list.
     *
     * @return void
     */
    public function __construct()
    {
        $this->clear();
    }

    /**
     * Clear the contents of the list and reset the head node and current node.
     *
     * @return void
     */
    public function clear()
    {
        $this->head = new ClassNode();
        $this->current = $this->head;
    }

    /**
     * Traverse to the next node in the list.
     *
     * @return void
     */
    public function next()
    {
        if (isset($this->current->next)) {
            $this->current = $this->current->next;
        } else {
            $this->current->next = new ClassNode(null, $this->current);
            $this->current = $this->current->next;
        }
    }

    /**
     * Insert a value at the current position in the list.
     *
     * Any currently set value at this position will be pushed back in the list
     * after the new value.
     *
     * @param mixed $value
     *
     * @return void
     */
    public function push($value)
    {
        if (!$this->current->value) {
            $this->current->value = $value;
        } else {
            $temp = $this->current;
            $this->current = new ClassNode($value, $temp->prev);
            $this->current->next = $temp;
            $temp->prev = $this->current;
            if ($temp === $this->head) {
                $this->head = $this->current;
            } else {
                $this->current->prev->next = $this->current;
            }
        }
    }

    /**
     * Traverse the ClassList and return a list of classes.
     *
     * @return array
     */
    public function getClasses()
    {
        $classes = array();
        $current = $this->head;
        while ($current && $current->value) {
            $classes[] = $current->value;
            $current = $current->next;
        }

        return array_filter($classes);
    }
}
