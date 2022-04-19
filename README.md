# KLights

Individually addressable LED project using SK6812s, ESP2866 (NodeMCU), and Home Assistant.
We'll see what it turns into.

### configuration.yaml

Currently 100% optimistic but maybe that'll change at some point.

```
light:
  - platform: mqtt
    name: 'Kitchen Lights'
    schema: json
    command_topic: 'home/lights/kitchen/set'
    brightness: true
    brightness_scale: 100
    color_mode: true
    effect: true
    supported_color_modes: [hs]
    retain: true  # for now?
```
