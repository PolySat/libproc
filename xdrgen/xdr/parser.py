# This grammar is written using Pyparsing and closely follows the grammar in
# RFC 4506.
#
# Known differences with the standard:
#  - We don't allow inline struct, enum, and union definitions.
#  - The standard is inconsistent on whether identifiers are allowed as enum
#    values. We only support constants as enum values.

## Grammar

import pyparsing as P
from xdr.ir import *

kw = P.Keyword
s = P.Suppress
lit = P.Literal
g = P.Group
namespace = 'FOO_'
enumNamespace = ''
scopeMap = {}

def namespaceParse(t):
   global namespace
   if t[1].upper() == 'IPC':
      namespace = 'IPC::'
   else:
      namespace = 'IPC::' + t[1].upper() + '::'
#print("Namespace is now " + namespace)
   return t

def scopeIdent(t):
   return namespace + t[0]

def scopeIdentDecl(t):
   global scopeMap
   scopeMap[t[0].lower()] = namespace + t[0]
   return namespace + t[0]

def scopeIdentUpper(t):
   return namespace + t[0].upper()

def enumStart(t):
   global enumNamespace
   global scopeMap
   enumNamespace = namespace + t[0].upper() + '::'
   scopeMap[t[0].lower()] = namespace + t[0].upper()
   return namespace + t[0].upper()

def enumIdent(t):
   if isinstance(t[0], str):
      global scopeMap
      name = enumNamespace + str(t[0]).upper()
      scopeMap[name[len(namespace):].lower()] = name
      return name
   return t[0]

def newStruct(t):
   global scopeMap
   scopeMap[t[1].split('::')[-1].lower()] = t[1]

def resolveIdent(t):
   global scopeMap
   return scopeMap[t[0].lower()]

# -:  self + And._ErrorStop() + other
identifier = P.Word(P.alphanums, P.alphanums + '_').setName("identifier")

scopedidentifier = P.Word(P.alphanums, P.alphanums + '_')
scopedidentifier.setParseAction(scopeIdent)

newscopedidentifier = P.Word(P.alphanums, P.alphanums + '_')
newscopedidentifier.setParseAction(scopeIdentDecl)

scopedUpperIdentifier = P.Word(P.alphanums, P.alphanums + '_')
scopedUpperIdentifier.setParseAction(scopeIdentUpper)

enumIdentifier = P.Word(P.alphanums, P.alphanums + '_')
enumIdentifier.setParseAction(enumStart)

enumValueIdentifier = P.Word(P.alphanums, P.alphanums + '_')
enumValueIdentifier.setParseAction(enumIdent)

resolvedIdentifier = P.Word(P.alphanums, P.alphanums + '_')
resolvedIdentifier.setParseAction(resolveIdent)

type_name = P.Combine(identifier +  P.OneOrMore('::' + identifier))
type_name.setParseAction(resolveIdent)

decimal_constant = P.Word('-123456789', '0123456789') | '0'
hexadecimal_constant = P.Combine('0x' - P.Word('0123456789abcdefABCDEF'))
octal_constant = P.Word('0', '01234567')

constant = (hexadecimal_constant | decimal_constant) \
    .setName("constant") \
    .setParseAction(lambda x: int(x[0], 0))

value = constant | identifier

struct_body = P.Forward()

type_specifier = \
    (P.Optional(kw("unsigned")) + kw("int")) | \
    (P.Optional(kw("unsigned")) + kw("hyper")) | \
    kw("float") | kw("double") | kw("quadruple") | kw("bool") | \
    s(kw("struct")) + struct_body | \
    identifier

resolved_type_specifier = \
    (P.Optional(kw("unsigned")) + kw("int")) | \
    (P.Optional(kw("unsigned")) + kw("hyper")) | \
    kw("float") | kw("double") | kw("quadruple") | kw("bool") | \
    s(kw("struct")) + struct_body | \
    resolvedIdentifier

case_type_specifier = \
    (P.Optional(kw("unsigned")) + kw("int")) | \
    (P.Optional(kw("unsigned")) + kw("hyper")) | \
    kw("float") | kw("double") | kw("quadruple") | kw("bool") | \
    scopedidentifier

declaration = \
    kw("void") | \
    kw("opaque") + identifier + lit("[") + value + lit("]") | \
    kw("opaque") + identifier + lit("<") + P.Optional(value) + lit(">") | \
    kw("string") + identifier + lit("<") + P.Optional(value) + lit(">") | \
    g(resolved_type_specifier) + identifier + lit("[") + value + lit("]") | \
    g(resolved_type_specifier) + identifier + lit("<") + P.Optional(value) + lit(">") | \
    g(resolved_type_specifier) + identifier | \
    g(resolved_type_specifier) + lit('*') + identifier

fielddocumentation = \
    s("{") + (P.Optional(g(kw("key") + identifier + s(";"))) & \
       P.Optional(g(kw("name") + P.QuotedString('"') + s(";"))) & \
       P.Optional(g(kw("unit") + P.QuotedString('"') + s(";"))) & \
       P.Optional(g(kw("offset") + decimal_constant + s(";"))) & \
       P.Optional(g(kw("divisor") + decimal_constant + s(";"))) & \
       P.Optional(g(kw("description") + P.QuotedString('"') + s(";"))) \
    ) + s("}")


const_expr_value = constant | scopedidentifier
constant_expr = (const_expr_value + (kw('+') | kw('-')) + const_expr_value) | const_expr_value
enum_body = s("{") + g(P.delimitedList(g(enumValueIdentifier + s('=') + constant_expr))) + s("}")

struct_body << s("{") + P.OneOrMore(g(declaration + g(P.Optional(fielddocumentation)) + s(";"))) + s("}")

constant_def = kw("const") - scopedUpperIdentifier + s("=") + constant + s(";")
namespace_def = kw("namespace") - identifier + s(";")
namespace_def.setParseAction(namespaceParse)

struct_def = kw("struct") - newscopedidentifier + g(struct_body) + P.Optional(s('=') + type_name) + s(";")
struct_def.setParseAction(newStruct)

command_options = \
       P.Optional(g(kw("summary") + P.QuotedString('"') + s(";"))) & \
       P.Optional(g(kw("param") + type_name + s(";")))  & \
       P.Optional(g(kw("response") + g(type_name + P.ZeroOrMore(s(',') + type_name)) + s(";")))

command_body = s("{") + command_options + s("}")

command_def = kw("command") + P.QuotedString('"') + g(command_body) + s('=') + type_name + s(";")

union_def = kw("union") - newscopedidentifier + s('{') + s('}') + s(";")

error_def = kw("error") + type_name + s('=') + P.QuotedString('"') + s(";")

type_def = \
    kw("enum") - enumIdentifier + enum_body + s(";") | \
    union_def | \
    struct_def | \
    command_def | \
    error_def

import_def = kw("import") + P.QuotedString('"') + s(";")

definition = type_def | constant_def | import_def

specification = g(namespace_def) & P.ZeroOrMore(g(definition))

specification.ignore(P.cStyleComment)

def xdr_parse_ast(src):
    """
    Given an input string, return the AST.
    """
    return specification.parseString(src, parseAll=True).asList()

def xdr_parse_fielddoc(x):
    key = ''
    name = ''
    desc = ''
    offset = divisor = 0
    unit = ''
    for field in x:
        if field[0] == 'key':
            key = field[1]
        if field[0] == 'name':
            name = field[1]
        if field[0] == 'description':
            desc = field[1]
        if field[0] == 'unit':
            unit = field[1]
        if field[0] == 'offset':
            offset = field[1]
        if field[0] == 'divisor':
            divisor = field[1]
    return XDRFieldDocumentation(key, name, desc, offset, divisor, unit)

def xdr_parse_declaration(x):
    if x[0] == 'void':
        return XDRDeclaration(None, 'basic', 'void', None, None, XDRFieldDocumentation('', '', '', 0, 0, ''))
    elif x[0] == 'opaque' or x[0] == 'string':
        type = x[0]
        name = x[1]
        kind = x[2] == '[' and 'array' or 'list'
        length = x[3]
        docs = None
        if length == '>':
            length = None
            if len(x) > 4:
               docs = xdr_parse_fielddoc(x[4])
        else:
            if len(x) > 5:
               docs = xdr_parse_fielddoc(x[5])

        if length == None:
            length_type = ''
        else:
            try:
                intlen = int(length)
                length_type = 'fixed'
            except:
               length_type = 'variable'
        if type == 'string':
            length_type = 'variable'

        return XDRDeclaration(name, kind, type, length, length_type, docs)
    else:
        name = x[1]
        docidx = 2
        kind = 'basic'
        type = ' '.join(x[0])
        length = None
        if name == '*':
            name = x[2]
            kind = 'optional'
            docidx = 3
        elif len(x) > 2 and x[2] == '[':
            kind = 'array'
        elif len(x) > 2 and x[2] == '<':
            kind = 'list'
        if kind == 'array' or kind == 'list':
            length = x[3]
            docidx = 5
            if length == '>':
                length = None
                docidx = 4
        doc = None
        if len(x) > docidx:
            doc = xdr_parse_fielddoc(x[docidx])
        if length == None:
            length_type = ''
        else:
            try:
                intlen = int(length)
                length_type = 'fixed'
            except:
               length_type = 'variable'
        return XDRDeclaration(name, kind, type, length, length_type, doc)

def xdr_parse_definition(x):
    if x[0] == 'namespace':
        return XDRNamespace(x[1])
    elif x[0] == 'struct':
        sid = None
        if len(x) == 4:
            sid = x[3]
        return XDRStruct(x[1], [xdr_parse_declaration(y) for y in x[2]], sid)
    elif x[0] == 'enum':
        return XDREnum(x[1], [XDREnumMember(y[0], ' '.join(str(e) for e in y[1:])) for y in x[2]])
    elif x[0] == 'const':
        return XDRConst(x[1], ' '.join(str(e) for e in x[2:]))
    elif x[0] == 'typedef':
        return XDRTypedef(xdr_parse_declaration(x[1]))
    elif x[0] == 'union':
        return XDRUnion(x[1], None, [])
#        return XDRUnion(x[1], xdr_parse_declaration(x[2]),
#                        [XDRUnionMember(y[0], xdr_parse_declaration(y[1])) for y in x[3]])
    elif x[0] == 'error':
        return XDRError(x[1], x[1].split('::')[-1], x[2])

    elif x[0] == 'command':
        summary = None
        param = '0'
        response = None
        for field in x[2]:
            if field[0] == 'summary':
                summary = field[1]
            if field[0] == 'param':
                param = field[1]
            if field[0] == 'response':
                response = field[1]
        return XDRCommand(x[1], x[3], summary, param, response)

def xdr_parse(src):
    """
    Given an input string, return the IR.
    """
    global scopeMap
#Enums defined in the parent IPC scope
    scopeMap['ipc::commands::status'] = 'IPC::COMMANDS::STATUS';
    scopeMap['ipc::commands::data_req'] = 'IPC::COMMANDS::DATA_REQ';
    scopeMap['ipc::datareq'] = 'IPC::DataReq';

    ast = xdr_parse_ast(src)
#print(ast)
#print(scopeMap)
    return [xdr_parse_definition(x) for x in ast]
