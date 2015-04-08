<?php namespace Illuminate\Support;

use Closure;
use Countable;
use ArrayAccess;
use ArrayIterator;
use CachingIterator;
use JsonSerializable;
use IteratorAggregate;
use Illuminate\Contracts\Support\Jsonable;
use Illuminate\Contracts\Support\Arrayable;

class Collection implements ArrayAccess, Arrayable, Countable, IteratorAggregate, Jsonable, JsonSerializable {

	/**
	 * The items contained in the collection.
	 *
	 * @var array
	 */
	protected $items = array();

	/**
	 * Create a new collection.
	 *
	 * @param  mixed  $items
	 * @return void
	 */
	public function __construct($items = array())
	{
		$items = is_null($items) ? [] : $this->getArrayableItems($items);

		$this->items = (array) $items;
	}

	/**
	 * Create a new collection instance if the value isn't one already.
	 *
	 * @param  mixed  $items
	 * @return static
	 */
	public static function make($items = null)
	{
		return new static($items);
	}

	/**
	 * Get all of the items in the collection.
	 *
	 * @return array
	 */
	public function all()
	{
		return $this->items;
	}

	/**
	 * Collapse the collection items into a single array.
	 *
	 * @return static
	 */
	public function collapse()
	{
		$results = [];

		foreach ($this->items as $values)
		{
			if ($values instanceof Collection) $values = $values->all();

			$results = array_merge($results, $values);
		}

		return new static($results);
	}

	/**
	 * Determine if an item exists in the collection.
	 *
	 * @param  mixed  $key
	 * @param  mixed  $value
	 * @return bool
	 */
	public function contains($key, $value = null)
	{
		if (func_num_args() == 2)
		{
			return $this->contains(function($k, $item) use ($key, $value)
			{
				return data_get($item, $key) == $value;
			});
		}

		if (is_callable($key))
		{
			return ! is_null($this->first($key));
		}

		return in_array($key, $this->items);
	}

	/**
	 * Diff the collection with the given items.
	 *
	 * @param  \Illuminate\Support\Collection|\Illuminate\Contracts\Support\Arrayable|array  $items
	 * @return static
	 */
	public function diff($items)
	{
		return new static(array_diff($this->items, $this->getArrayableItems($items)));
	}

	/**
	 * Execute a callback over each item.
	 *
	 * @param  callable  $callback
	 * @return $this
	 */
	public function each(callable $callback)
	{
		array_map($callback, $this->items);

		return $this;
	}

	/**
	 * Fetch a nested element of the collection.
	 *
	 * @param  string  $key
	 * @return static
	 */
	public function fetch($key)
	{
		return new static(array_fetch($this->items, $key));
	}

	/**
	 * Run a filter over each of the items.
	 *
	 * @param  callable  $callback
	 * @return static
	 */
	public function filter(callable $callback)
	{
		return new static(array_filter($this->items, $callback));
	}

	/**
	 * Filter items by the given key value pair.
	 *
	 * @param  string  $key
	 * @param  mixed  $value
	 * @param  bool  $strict
	 * @return static
	 */
	public function where($key, $value, $strict = true)
	{
		return $this->filter(function($item) use ($key, $value, $strict)
		{
			return $strict ? data_get($item, $key) === $value
                           : data_get($item, $key) == $value;
		});
	}

	/**
	 * Filter items by the given key value pair using loose comparison.
	 *
	 * @param  string  $key
	 * @param  mixed  $value
	 * @return static
	 */
	public function whereLoose($key, $value)
	{
		return $this->where($key, $value, false);
	}

	/**
	 * Get the first item from the collection.
	 *
	 * @param  callable   $callback
	 * @param  mixed      $default
	 * @return mixed|null
	 */
	public function first(callable $callback = null, $default = null)
	{
		if (is_null($callback))
		{
			return count($this->items) > 0 ? reset($this->items) : null;
		}

		return array_first($this->items, $callback, $default);
	}

	/**
	 * Get a flattened array of the items in the collection.
	 *
	 * @return static
	 */
	public function flatten()
	{
		return new static(array_flatten($this->items));
	}

	/**
	 * Flip the items in the collection.
	 *
	 * @return static
	 */
	public function flip()
	{
		return new static(array_flip($this->items));
	}

	/**
	 * Remove an item from the collection by key.
	 *
	 * @param  mixed  $key
	 * @return void
	 */
	public function forget($key)
	{
		$this->offsetUnset($key);
	}

	/**
	 * Get an item from the collection by key.
	 *
	 * @param  mixed  $key
	 * @param  mixed  $default
	 * @return mixed
	 */
	public function get($key, $default = null)
	{
		if ($this->offsetExists($key))
		{
			return $this->items[$key];
		}

		return value($default);
	}

	/**
	 * Group an associative array by a field or using a callback.
	 *
	 * @param  callable|string  $groupBy
	 * @return static
	 */
	public function groupBy($groupBy)
	{
		if ( ! $this->useAsCallable($groupBy))
		{
			return $this->groupBy($this->valueRetriever($groupBy));
		}

		$results = [];

		foreach ($this->items as $key => $value)
		{
			$results[$groupBy($value, $key)][] = $value;
		}

		return new static($results);
	}

	/**
	 * Key an associative array by a field or using a callback.
	 *
	 * @param  callable|string  $keyBy
	 * @return static
	 */
	public function keyBy($keyBy)
	{
		if ( ! $this->useAsCallable($keyBy))
		{
			return $this->keyBy($this->valueRetriever($keyBy));
		}

		$results = [];

		foreach ($this->items as $item)
		{
			$results[$keyBy($item)] = $item;
		}

		return new static($results);
	}

	/**
	 * Determine if an item exists in the collection by key.
	 *
	 * @param  mixed  $key
	 * @return bool
	 */
	public function has($key)
	{
		return $this->offsetExists($key);
	}

	/**
	 * Concatenate values of a given key as a string.
	 *
	 * @param  string  $value
	 * @param  string  $glue
	 * @return string
	 */
	public function implode($value, $glue = null)
	{
		$first = $this->first();

		if (is_array($first) || is_object($first))
		{
			return implode($glue, $this->lists($value));
		}

		return implode($value, $this->items);
	}

	/**
	 * Intersect the collection with the given items.
	 *
 	 * @param  \Illuminate\Support\Collection|\Illuminate\Contracts\Support\Arrayable|array  $items
	 * @return static
	 */
	public function intersect($items)
	{
		return new static(array_intersect($this->items, $this->getArrayableItems($items)));
	}

	/**
	 * Determine if the collection is empty or not.
	 *
	 * @return bool
	 */
	public function isEmpty()
	{
		return empty($this->items);
	}

	/**
	 * Determine if the given value is callable, but not a string.
	 *
	 * @param  mixed  $value
	 * @return bool
	 */
	protected function useAsCallable($value)
	{
		return ! is_string($value) && is_callable($value);
	}

	/**
	 * Get the keys of the collection items.
	 *
	 * @return static
	 */
	public function keys()
	{
		return new static(array_keys($this->items));
	}

	/**
	* Get the last item from the collection.
	*
	* @return mixed|null
	*/
	public function last()
	{
		return count($this->items) > 0 ? end($this->items) : null;
	}

	/**
	 * Get an array with the values of a given key.
	 *
	 * @param  string  $value
	 * @param  string  $key
	 * @return array
	 */
	public function lists($value, $key = null)
	{
		return array_pluck($this->items, $value, $key);
	}

	/**
	 * Run a map over each of the items.
	 *
	 * @param  callable  $callback
	 * @return static
	 */
	public function map(callable $callback)
	{
		return new static(array_map($callback, $this->items, array_keys($this->items)));
	}

	/**
	 * Merge the collection with the given items.
	 *
	 * @param  \Illuminate\Support\Collection|\Illuminate\Contracts\Support\Arrayable|array  $items
	 * @return static
	 */
	public function merge($items)
	{
		return new static(array_merge($this->items, $this->getArrayableItems($items)));
	}

	/**
	 * "Paginate" the collection by slicing it into a smaller collection.
	 *
	 * @param  int  $page
	 * @param  int  $perPage
	 * @return static
	 */
	public function forPage($page, $perPage)
	{
		return new static(array_slice($this->items, ($page - 1) * $perPage, $perPage));
	}

	/**
	 * Get and remove the last item from the collection.
	 *
	 * @return mixed|null
	 */
	public function pop()
	{
		return array_pop($this->items);
	}

	/**
	 * Push an item onto the beginning of the collection.
	 *
	 * @param  mixed  $value
	 * @return void
	 */
	public function prepend($value)
	{
		array_unshift($this->items, $value);
	}

	/**
	 * Push an item onto the end of the collection.
	 *
	 * @param  mixed  $value
	 * @return void
	 */
	public function push($value)
	{
		$this->offsetSet(null, $value);
	}

	/**
	 * Pulls an item from the collection.
	 *
	 * @param  mixed  $key
	 * @param  mixed  $default
	 * @return mixed
	 */
	public function pull($key, $default = null)
	{
		return array_pull($this->items, $key, $default);
	}

	/**
	 * Put an item in the collection by key.
	 *
	 * @param  mixed  $key
	 * @param  mixed  $value
	 * @return void
	 */
	public function put($key, $value)
	{
		$this->offsetSet($key, $value);
	}

	/**
	 * Get one or more items randomly from the collection.
	 *
	 * @param  int  $amount
	 * @return mixed
	 */
	public function random($amount = 1)
	{
		if ($this->isEmpty()) return;

		$keys = array_rand($this->items, $amount);

		return is_array($keys) ? array_intersect_key($this->items, array_flip($keys)) : $this->items[$keys];
	}

	/**
	 * Reduce the collection to a single value.
	 *
	 * @param  callable  $callback
	 * @param  mixed     $initial
	 * @return mixed
	 */
	public function reduce(callable $callback, $initial = null)
	{
		return array_reduce($this->items, $callback, $initial);
	}

	/**
	 * Create a collection of all elements that do not pass a given truth test.
	 *
	 * @param  callable|mixed  $callback
	 * @return static
	 */
	public function reject($callback)
	{
		if ($this->useAsCallable($callback))
		{
			return $this->filter(function($item) use ($callback)
			{
				return ! $callback($item);
			});
		}

		return $this->filter(function($item) use ($callback)
		{
			return $item != $callback;
		});
	}

	/**
	 * Reverse items order.
	 *
	 * @return static
	 */
	public function reverse()
	{
		return new static(array_reverse($this->items));
	}

	/**
	 * Search the collection for a given value and return the corresponding key if successful.
	 *
	 * @param  mixed  $value
	 * @param  bool   $strict
	 * @return mixed
	 */
	public function search($value, $strict = false)
	{
		return array_search($value, $this->items, $strict);
	}

	/**
	 * Get and remove the first item from the collection.
	 *
	 * @return mixed|null
	 */
	public function shift()
	{
		return array_shift($this->items);
	}

	/**
	 * Shuffle the items in the collection.
	 *
	 * @return $this
	 */
	public function shuffle()
	{
		shuffle($this->items);

		return $this;
	}

	/**
	 * Slice the underlying collection array.
	 *
	 * @param  int   $offset
	 * @param  int   $length
	 * @param  bool  $preserveKeys
	 * @return static
	 */
	public function slice($offset, $length = null, $preserveKeys = false)
	{
		return new static(array_slice($this->items, $offset, $length, $preserveKeys));
	}

	/**
	 * Chunk the underlying collection array.
	 *
	 * @param  int   $size
	 * @param  bool  $preserveKeys
	 * @return static
	 */
	public function chunk($size, $preserveKeys = false)
	{
		$chunks = [];

		foreach (array_chunk($this->items, $size, $preserveKeys) as $chunk)
		{
			$chunks[] = new static($chunk);
		}

		return new static($chunks);
	}

	/**
	 * Sort through each item with a callback.
	 *
	 * @param  callable  $callback
	 * @return $this
	 */
	public function sort(callable $callback)
	{
		uasort($this->items, $callback);

		return $this;
	}

	/**
	 * Sort the collection using the given callback.
	 *
	 * @param  callable|string  $callback
	 * @param  int   $options
	 * @param  bool  $descending
	 * @return $this
	 */
	public function sortBy($callback, $options = SORT_REGULAR, $descending = false)
	{
		$results = [];

		if ( ! $this->useAsCallable($callback))
		{
			$callback = $this->valueRetriever($callback);
		}

		// First we will loop through the items and get the comparator from a callback
		// function which we were given. Then, we will sort the returned values and
		// and grab the corresponding values for the sorted keys from this array.
		foreach ($this->items as $key => $value)
		{
			$results[$key] = $callback($value);
		}

		$descending ? arsort($results, $options)
                    : asort($results, $options);

		// Once we have sorted all of the keys in the array, we will loop through them
		// and grab the corresponding model so we can set the underlying items list
		// to the sorted version. Then we'll just return the collection instance.
		foreach (array_keys($results) as $key)
		{
			$results[$key] = $this->items[$key];
		}

		$this->items = $results;

		return $this;
	}

	/**
	 * Sort the collection in descending order using the given callback.
	 *
	 * @param  callable|string  $callback
	 * @param  int  $options
	 * @return $this
	 */
	public function sortByDesc($callback, $options = SORT_REGULAR)
	{
		return $this->sortBy($callback, $options, true);
	}

	/**
	 * Splice portion of the underlying collection array.
	 *
	 * @param  int    $offset
	 * @param  int    $length
	 * @param  mixed  $replacement
	 * @return static
	 */
	public function splice($offset, $length = 0, $replacement = [])
	{
		return new static(array_splice($this->items, $offset, $length, $replacement));
	}

	/**
	 * Get the sum of the given values.
	 *
	 * @param  callable|string|null  $callback
	 * @return mixed
	 */
	public function sum($callback = null)
	{
		if (is_null($callback))
		{
			return array_sum($this->items);
		}

		if ( ! $this->useAsCallable($callback))
		{
			$callback = $this->valueRetriever($callback);
		}

		return $this->reduce(function($result, $item) use ($callback)
		{
			return $result += $callback($item);
		}, 0);
	}

	/**
	 * Take the first or last {$limit} items.
	 *
	 * @param  int  $limit
	 * @return static
	 */
	public function take($limit = null)
	{
		if ($limit < 0) return $this->slice($limit, abs($limit));

		return $this->slice(0, $limit);
	}

	/**
	 * Transform each item in the collection using a callback.
	 *
	 * @param  callable  $callback
	 * @return $this
	 */
	public function transform(callable $callback)
	{
		$this->items = array_map($callback, $this->items);

		return $this;
	}

	/**
	 * Return only unique items from the collection array.
	 *
	 * @return static
	 */
	public function unique()
	{
		return new static(array_unique($this->items));
	}

	/**
	 * Reset the keys on the underlying array.
	 *
	 * @return static
	 */
	public function values()
	{
		return new static(array_values($this->items));
	}

	/**
	 * Get a value retrieving callback.
	 *
	 * @param  string  $value
	 * @return \Closure
	 */
	protected function valueRetriever($value)
	{
		return function($item) use ($value)
		{
			return data_get($item, $value);
		};
	}

	/**
	 * Get the collection of items as a plain array.
	 *
	 * @return array
	 */
	public function toArray()
	{
		return array_map(function($value)
		{
			return $value instanceof Arrayable ? $value->toArray() : $value;

		}, $this->items);
	}

	/**
	 * Convert the object into something JSON serializable.
	 *
	 * @return array
	 */
	public function jsonSerialize()
	{
		return $this->toArray();
	}

	/**
	 * Get the collection of items as JSON.
	 *
	 * @param  int  $options
	 * @return string
	 */
	public function toJson($options = 0)
	{
		return json_encode($this->toArray(), $options);
	}

	/**
	 * Get an iterator for the items.
	 *
	 * @return \ArrayIterator
	 */
	public function getIterator()
	{
		return new ArrayIterator($this->items);
	}

	/**
	 * Get a CachingIterator instance.
	 *
	 * @param  int  $flags
	 * @return \CachingIterator
	 */
	public function getCachingIterator($flags = CachingIterator::CALL_TOSTRING)
	{
		return new CachingIterator($this->getIterator(), $flags);
	}

	/**
	 * Count the number of items in the collection.
	 *
	 * @return int
	 */
	public function count()
	{
		return count($this->items);
	}

	/**
	 * Determine if an item exists at an offset.
	 *
	 * @param  mixed  $key
	 * @return bool
	 */
	public function offsetExists($key)
	{
		return array_key_exists($key, $this->items);
	}

	/**
	 * Get an item at a given offset.
	 *
	 * @param  mixed  $key
	 * @return mixed
	 */
	public function offsetGet($key)
	{
		return $this->items[$key];
	}

	/**
	 * Set the item at a given offset.
	 *
	 * @param  mixed  $key
	 * @param  mixed  $value
	 * @return void
	 */
	public function offsetSet($key, $value)
	{
		if (is_null($key))
		{
			$this->items[] = $value;
		}
		else
		{
			$this->items[$key] = $value;
		}
	}

	/**
	 * Unset the item at a given offset.
	 *
	 * @param  string  $key
	 * @return void
	 */
	public function offsetUnset($key)
	{
		unset($this->items[$key]);
	}

	/**
	 * Convert the collection to its string representation.
	 *
	 * @return string
	 */
	public function __toString()
	{
		return $this->toJson();
	}

	/**
	 * Results array of items from Collection or Arrayable.
	 *
  	 * @param  \Illuminate\Support\Collection|\Illuminate\Contracts\Support\Arrayable|array  $items
	 * @return array
	 */
	protected function getArrayableItems($items)
	{
		if ($items instanceof Collection)
		{
			$items = $items->all();
		}
		elseif ($items instanceof Arrayable)
		{
			$items = $items->toArray();
		}

		return $items;
	}

}
