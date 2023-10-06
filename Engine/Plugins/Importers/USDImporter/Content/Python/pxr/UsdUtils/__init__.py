#
# Copyright 2016 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#
import os
import platform
if platform.system() == 'Darwin':
    from .. import importutils
    curent_path = os.path.dirname(__file__)
    _usdUtils = importutils.dynamic_import('pxr.UsdUtils._usdUtils', os.path.join(curent_path, 'mac', '_usdUtils.so'))
else:
    from . import _usdUtils

from pxr import Tf
Tf.PrepareModule(_usdUtils, locals())
del _usdUtils, Tf

try:
    from . import __DOC
    __DOC.Execute(locals())
    del __DOC
except Exception:
    pass


from .complianceChecker import ComplianceChecker
from .updateSchemaWithSdrNode import UpdateSchemaWithSdrNode, \
        SchemaDefiningKeys, SchemaDefiningMiscConstants, PropertyDefiningKeys
