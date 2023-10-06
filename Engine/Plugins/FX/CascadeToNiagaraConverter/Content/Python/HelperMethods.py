def find_by_pred(elements, pred):
	"""
	Search a list of arbitrary typed elements and find an entry that passes
	the predicate function argument.

	Args:
		elements (object[]): List of objects to search.
		pred (function): Function that takes one argument bbject and returns
	True if the bbject argument is the desired entry in the elements list.

	Returns:
		Object: Object entry from the elements list that passes the pred
	function, or None if no entry in the elements list passes pred.
	"""

	for i, v in enumerate(elements):
		if pred(v):
			return v
	return None


def class_has_parent(target_class, target_parent_class):
	"""
	Check if a class has a specific parent class.

	Args:
		target_class (class): Class to check if it has a specific parent.
		target_parent_class (class): Class to compare to parent classes of
	target_class.

	Returns:
		bool: True if the target_class has target_parent_class as a parent.
	"""

	parent_subclasses = set()
	classes_to_check = [target_parent_class]
	while classes_to_check:
		parent_class = classes_to_check.pop()
		for child_class in parent_class.__subclasses__():
			if child_class not in parent_subclasses:
				parent_subclasses.add(child_class)
				classes_to_check.append(child_class)
	return target_class in parent_subclasses
