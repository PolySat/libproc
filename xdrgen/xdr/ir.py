"""
Intermediate Representation (IR)

This file is intended to be imported as "from xdr.ir import *".
"""

from collections import namedtuple

XDRStruct = namedtuple("XDRStruct", ["name", "members", "id"])
XDRDeclaration = namedtuple("XDRDeclaration", ["name", "kind", "type", "length", "length_type", "documentation"])
XDREnum = namedtuple("XDREnum", ["name", "members"])
XDREnumMember = namedtuple("XDREnumMember", ["name", "value"])
XDRConst = namedtuple("XDRConst", ["name", "value"])
XDRTypedef = namedtuple("XDRTypedef", ["declaration"])
XDRUnion = namedtuple("XDRUnion", ["name", "discriminant", "members"])
XDRUnionMember = namedtuple("XDRUnionMember", ["case", "declaration"])
XDRNamespace = namedtuple("XDRNamespace", ["name"])
XDRFieldDocumentation = namedtuple("XDRFieldDocs", ["key", "name", "description", "offset", "divisor", "unit"])
XDRCommand = namedtuple("XDRCommand", ["name", "id", "summary", "param", "response"])
XDRError = namedtuple("XDRError", ["id", "name", "description"])

__all__ = [
    'XDRStruct', 'XDRDeclaration', 'XDREnum', 'XDREnumMember',
    'XDRConst', 'XDRTypedef', 'XDRUnion', 'XDRUnionMember',
    'XDRNamespace', 'XDRFieldDocumentation', 'XDRCommand', 'XDRError'
]
