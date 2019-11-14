require './genutil.rb'

events = {}
YAML.load_file('events.yml').to_o.events.each do |e|
    actions = {}
    e.actions.each do |a|
        actions[a.name] = a
    end
    e.actions = actions
    events[e.domain] = e
end

scheme = YAML.load_file('drivers.yml').to_o

includes = []
scheme.drivers.each do |d|
    d.args = d.args || []
    d.includes = (d.includes || []).uniq
    includes = includes.concat(d.includes)
    d.events.each do |l|
        l.actions = l.actions.map do |a|
            action = events[l.domain].actions[a]
            raise "unknown action #{l.domain}.#{a}, referenced by #{d.name} driver" unless action
            action.args.each do |arg|
                if arg.type.is_a? Array
                    decl = "#{ l.domain }_event_#{ action.name }_#{ arg.name }"
                    arg.type = "#{ decl }_t"
                end
            end
            action
        end
    end

    d.decl = "#{ d.name }_driver"
    d.ref = "#{ d.decl }_ref"

    ErbalT.new(d).render("driver_plug.template.c", "#{ d.name }_driver_plug.c")
    ErbalT.new(d).render("driver_impl.template.h", "#{ d.name }_driver_impl.h")
end

scheme.includes = includes.uniq

ErbalT.new(scheme).render("drivers.template.h")
