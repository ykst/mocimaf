#include "drivers.h"
#include "<%= name %>_driver_impl.h"

<%= ref %> <%= decl %>_plug(core_ref core<%= expand_cargs(args) %>)
{
    <%= ref %> driver = NULL;

    GUARD(driver = <%= decl %>_create(core<%= expand_cparams(args) %>));

<% events.each do |l| -%>
    <%= l.domain %>_event_attach_listener(&core->events.<%= l.domain %>,
        &(<%= l.domain %>_event_listener_t) {
            .context = driver,
        <% l.actions.each do |a| -%>
            .on_<%= a.name %> = (<%= l.domain %>_event_callback_on_<%= a.name %>_t)<%= decl %>_on_<%= l.domain %>_<%= a.name %>,
    <% end -%>
    });
<% end -%>

    core_event_attach_listener(&core->events.core, &(core_event_listener_t) {
        .context = driver,
        .on_destroy = (core_event_callback_on_destroy_t)<%= decl %>_destroy,
    });

    return driver;
error:
    <%= decl %>_destroy(driver);
    return NULL;
}

<% events.each do |l| -%>
<% l.actions.each do |a| -%>
void <%= decl %>_detach_<%= l.domain %>_<%= a.name %>(<%= ref %> self, core_ref core)
{
    <%= l.domain %>_event_emitter_t *emitter = &core->events.<%= l.domain %>;
    <%= l.domain %>_event_detach_listener_on_<%= a.name %>(emitter, self);
}
<% end -%>
<% end -%>

void <%= decl %>_unplug(core_ref core, <%= ref %> driver)
{
<% events.each do |l| -%>
<% l.actions.each do |a| -%>
    <%= decl %>_detach_<%= l.domain %>_<%= a.name %>(driver, core);
<% end -%>
<% end -%>
    core_event_detach_listener_on_destroy(&core->events.core, driver);

    <%= decl %>_destroy(driver);
}
