# asr_mission

Vehicle-agnostic mission planning: a plan *data structure* (not a textual
language) authored on the GCS, uploaded as an opaque blob through asr_comms,
and executed onboard. Control flow is a small fixed set of tree nodes;
capabilities are a small fixed set of per-vehicle skills and conditions.

**Status: skeleton only.** Every non-trivial function body currently throws
`std::logic_error("... not implemented")` rather than returning a plausible-
looking but fake result — an empty `validate()` or a `tick()` that claims
`Success` would be actively misleading to trust by accident. Each stub has a
`// TODO` comment describing the intended behavior.

## Layout

| Piece | Header | Source | Depends on |
|---|---|---|---|
| Plan data model + JSON | `include/asr_mission/plan.h` | `src/plan.cpp` | nlohmann_json |
| Skill/condition capabilities | `include/asr_mission/capabilities.h` | `src/capabilities.cpp` | yaml-cpp (+ TODO: a hashing lib for `hash()`) |
| Static validation | `include/asr_mission/plan_validator.h` | `src/plan_validator.cpp` | plan.h, capabilities.h |
| Tick-based executor | `include/asr_mission/plan_executor.h` | `src/plan_executor.cpp` | plan.h |
| Onboard ROS node | — | `src/mission_executor_node.cpp` | rclcpp, asr_comms (TODO) |

`plan.h`/`capabilities.h`/`plan_validator.h`/`plan_executor.h` have no
`rclcpp` types in their public interface — `asr_mission_lib` is meant to be
linked by both the onboard node here and the (future C++) GCS, so plan
authoring/validation code is identical on both ends.

One deliberate simplification worth knowing about: `TaskNode::params` is a
raw `nlohmann::json` object rather than a hand-rolled variant type — this
pulls `nlohmann/json.hpp` into the public header, which cuts against
generally keeping heavy third-party headers out of public interfaces, but
avoids inventing a parallel dynamic-value type for a template. Revisit if it
becomes a real cost.

## Plan model

Nodes: `task` (runs a skill), `sequence`, `retry(max_attempts)`,
`run_until(conditions_any)`. See `plans/example_probe_hunt.json` for the
canonical example (takeoff -> goto -> search grid until 3 probes found or
grid exhausted, retry twice -> land) — still plain JSON, no changes needed
for the C++ port.

Rules the validator is meant to enforce (see the TODO in
`plan_validator.cpp`): every loop is bounded (`retry` capped at
`kMaxRetryAttempts`, `run_until` must have conditions), every skill/param/
condition must exist in the target vehicle's capabilities and respect its
limits. The plan runs *under* the autopilot's failsafes (battery, geofence,
estop) — nothing in a plan can override those.

## Vehicle capabilities

Each vehicle package ships `config/skills.yaml` (see
`config/example_skills.yaml` for the format) declaring its skills, parameter
limits, and evaluable conditions. `discover_capabilities()` is meant to find
all installed manifests by scanning `AMENT_PREFIX_PATH`, so the GCS learns
about a vehicle by installing its package. The connect handshake carries
`vehicle` + `VehicleCapabilities::hash()`; a hash mismatch means GCS and
vehicle run different checkouts — warn before authoring.

## Upload / execution lifecycle (not yet implemented — see TODOs in
`mission_executor_node.cpp`)

1. **Upload** — plan blob chunked over the asr_comms link, CRC-checked
2. **Validate** — vehicle re-validates against its own capabilities, replies
   with the issue list; GCS-side validation is convenience only
3. **Start** — separate explicit command; never auto-start on upload
4. **Report** — executor publishes plan_id + `PlanExecutor::active_path()` +
   condition values so the GCS can highlight the program counter

## Building

```bash
colcon build --packages-select asr_mission
source install/setup.bash
ros2 run asr_mission mission_executor
```

No test scaffolding yet (gtest vs. Catch2 not decided).
