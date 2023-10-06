import math


def get_unique_name(used_names, prefix):
    """ Modifies `prefix` with numbered suffixes until it is not contained by used_names

    Does not insert the generated name into used_names.

    :param used_names: Collection of names that are not allowed (e.g. {"MyName", "MyName_0"})
    :param prefix: Name to make unique (e.g. "MyName")
    :returns: Modified `prefix` (e.g. "MyName_1")
    """
    suffix = 0
    test_name = prefix
    while test_name in used_names:
        test_name = prefix + "_" + str(suffix)
        suffix += 1

    return test_name


def clamp(val, min_val, max_val):
    return max(min(val, max_val), min_val)
