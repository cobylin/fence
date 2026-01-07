'use strict';
'use ui';

return L.view.extend({
    render: function() {
        var m, s, o;
        m = new form.Map('fence', _('Fence'), _('双栈自动识别流量拦截器'));

        s = m.section(form.TypedSection, 'database', _('拦截分组'));
        s.addremove = true;
        s.anonymous = true;

        s.option(form.Value, 'name', _('备注名称'));
        s.option(form.Flag, 'enabled', _('启用'));
        
        o = s.option(form.Value, 'path', _('MMDB 路径'));
        o.placeholder = '/etc/fence/mixed_db.mmdb';
        o.rmempty = false;

        o = s.option(form.Value, 'ratio', _('拦截比例 (0.0-1.0)'));
        o.default = '1.0';

        o = s.option(form.Value, 'queue', _('NFQueue ID'));
        o.default = '10';
        o.datatype = 'uinteger';

        return m.render();
    }
});
