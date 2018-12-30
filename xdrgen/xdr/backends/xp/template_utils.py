import sys
import os
import tenjin

class TemplateEngine(tenjin.Engine):
    # Copied from Tenjin with the addition of an indent before each output line
    def include_indented(self, template_name, indent, append_to_buf=True, **kwargs):
        assert append_to_buf
        #: get local and global vars of caller.
        frame = sys._getframe(1)
        locals  = frame.f_locals
        globals = frame.f_globals
        #: get _context from caller's local vars.
        assert '_context' in locals
        context = locals['_context']
        #: if kwargs specified then add them into context.
        if kwargs:
            context.update(kwargs)
        #: get template object with context data and global vars.
        ## (context and globals are passed to get_template() only for preprocessing.)
        template = self.get_template(template_name, context, globals)
        #: if append_to_buf is true then add output to _buf.
        #: if append_to_buf is false then don't add output to _buf.
        if append_to_buf:  _buf = locals['_buf']
        else:              _buf = None
        #: render template and return output.
        indent = ' ' * indent
        s = template.render(context, globals, _buf=None)
        for line in s.splitlines():
            if line == "":
                continue
            _buf.append(indent)
            _buf.append(line)
            _buf.append("\n")
        #: kwargs are removed from context data.
        if kwargs:
            for k in kwargs:
                del context[k]
        return s

    def hook_context(self, context):
        tenjin.Engine.hook_context(self, context)
        context['include_indented'] = self.include_indented

def render_template(out, name, context):
    """
    Render a template using tenjin.
    out: a file-like object
    name: name of the template
    context: dictionary of variables to pass to the template
    """
    path = [os.path.join(os.path.dirname(os.path.realpath(__file__)), 'templates')]
    pp = [ tenjin.PrefixedLinePreprocessor() ] # support "::" syntax
    template_globals = { "to_str": str, "escape": str } # disable HTML escaping
    engine = TemplateEngine(path=path, pp=pp)
    out.write(engine.render(name, context, template_globals))
