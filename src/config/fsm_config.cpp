#include "config/fsm_config.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>

#include "config/config_utils.h"
#include "interaction/rule/composite_rule.h"
#include "interaction/rule/single_rule.h"
#include "log/logging.h"
#include "utils/to_string.h"

namespace VisionCursor
{
namespace Config
{
namespace
{
using RulePtr = Interaction::Rule::RulePtr;

QJsonObject ruleBool(bool value)
{
    return QJsonObject{{"type", "BoolRule"}, {"value", value}};
}

QJsonObject ruleHandDetect(bool should_detect)
{
    return QJsonObject{{"type", "HandDetectRule"}, {"should_detect", should_detect}};
}

QJsonObject ruleDistance(HandJoint a, HandJoint b, CompareOp op, float threshold)
{
    return QJsonObject{{"type", "DistanceRule"},
                       {"joint_a", Utils::ToString(a)},
                       {"joint_b", Utils::ToString(b)},
                       {"compare_op", Utils::ToString(op)},
                       {"threshold", threshold}};
}

QJsonObject ruleMoveDistance(HandJoint joint, CompareOp op, float threshold, int frame_window)
{
    return QJsonObject{{"type", "MoveDistanceRule"},
                       {"joint", Utils::ToString(joint)},
                       {"compare_op", Utils::ToString(op)},
                       {"threshold", threshold},
                       {"frame_window", frame_window}};
}

QJsonObject ruleAngle(HandJoint a, HandJoint b, HandJoint c, CompareOp op, float threshold_degree)
{
    return QJsonObject{{"type", "AngleRule"},
                       {"joint_a", Utils::ToString(a)},
                       {"joint_b", Utils::ToString(b)},
                       {"joint_c", Utils::ToString(c)},
                       {"compare_op", Utils::ToString(op)},
                       {"threshold_degree", threshold_degree}};
}

QJsonObject ruleComposite(LogicOp op, const QJsonObject& left, const QJsonObject& right)
{
    return QJsonObject{
        {"type", "CompositeRule"}, {"logic_op", Utils::ToString(op)}, {"left_rule", left}, {"right_rule", right}};
}

QJsonObject ruleConsecutive(const QJsonObject& base, int required_frames)
{
    return QJsonObject{{"type", "ConsecutiveFrameRule"}, {"required_frames", required_frames}, {"base_rule", base}};
}

QJsonObject motionTransition(MotionState from, MotionState to, int priority, const QJsonObject& rule)
{
    return QJsonObject{
        {"from", Utils::ToString(from)}, {"to", Utils::ToString(to)}, {"priority", priority}, {"rule", rule}};
}

QJsonObject buttonTransition(ControlState from, ControlState to, int priority, const QJsonObject& rule)
{
    return QJsonObject{
        {"from", Utils::ToString(from)}, {"to", Utils::ToString(to)}, {"priority", priority}, {"rule", rule}};
}

QJsonObject buildEasyPresetJson()
{
    QJsonArray motion;
    QJsonArray button;

    motion.append(motionTransition(MotionState::Idle, MotionState::Ready, 10, ruleHandDetect(true)));
    motion.append(motionTransition(
        MotionState::Ready,
        MotionState::Moving,
        10,
        ruleConsecutive(ruleComposite(LogicOp::And,
                                      ruleAngle(HandJoint::IndexMCP,
                                                HandJoint::IndexPIP,
                                                HandJoint::IndexDIP,
                                                CompareOp::GreaterEqual,
                                                170.0f),
                                      ruleAngle(HandJoint::IndexPIP,
                                                HandJoint::IndexDIP,
                                                HandJoint::IndexTIP,
                                                CompareOp::GreaterEqual,
                                                170.0f)),
                        2)));
    motion.append(motionTransition(
        MotionState::Moving,
        MotionState::Ready,
        10,
        ruleConsecutive(ruleComposite(LogicOp::Or,
                                      ruleAngle(HandJoint::IndexMCP,
                                                HandJoint::IndexPIP,
                                                HandJoint::IndexDIP,
                                                CompareOp::LessEqual,
                                                165.0f),
                                      ruleAngle(HandJoint::IndexPIP,
                                                HandJoint::IndexDIP,
                                                HandJoint::IndexTIP,
                                                CompareOp::LessEqual,
                                                165.0f)),
                        2)));
    motion.append(motionTransition(MotionState::Ready, MotionState::Idle, 20, ruleHandDetect(false)));
    motion.append(motionTransition(MotionState::Moving, MotionState::Idle, 20, ruleHandDetect(false)));

    button.append(buttonTransition(ControlState::None,
                                   ControlState::LeftPressed,
                                   10,
                                   ruleConsecutive(
                                       ruleDistance(HandJoint::ThumbTIP, HandJoint::IndexTIP, CompareOp::LessEqual, 4.0f),
                                       2)));
    button.append(buttonTransition(
        ControlState::LeftPressed,
        ControlState::None,
        10,
        ruleConsecutive(ruleDistance(HandJoint::ThumbTIP, HandJoint::IndexTIP, CompareOp::GreaterEqual, 4.5f), 4)));
    button.append(buttonTransition(
        ControlState::None,
        ControlState::RightPressed,
        10,
        ruleConsecutive(ruleDistance(HandJoint::ThumbTIP, HandJoint::MiddleTIP, CompareOp::LessEqual, 4.0f), 2)));
    button.append(buttonTransition(
        ControlState::RightPressed,
        ControlState::None,
        10,
        ruleConsecutive(ruleDistance(HandJoint::ThumbTIP, HandJoint::MiddleTIP, CompareOp::GreaterEqual, 4.5f), 6)));
    button.append(buttonTransition(
        ControlState::None,
        ControlState::VerticalScrolling,
        5,
        ruleConsecutive(ruleDistance(HandJoint::ThumbTIP, HandJoint::RingTIP, CompareOp::LessEqual, 4.0f), 2)));
    button.append(buttonTransition(
        ControlState::VerticalScrolling,
        ControlState::None,
        5,
        ruleConsecutive(ruleDistance(HandJoint::ThumbTIP, HandJoint::RingTIP, CompareOp::GreaterEqual, 4.5f), 2)));

    return QJsonObject{{"motion_transitions", motion}, {"button_transitions", button}};
}

QJsonObject buildAdvancedPresetJson()
{
    QJsonArray motion;
    QJsonArray button;

    motion.append(motionTransition(MotionState::Idle, MotionState::Ready, 10, ruleHandDetect(true)));
    motion.append(motionTransition(
        MotionState::Ready,
        MotionState::Moving,
        10,
        ruleConsecutive(
            ruleComposite(
                LogicOp::And,
                ruleComposite(LogicOp::And,
                              ruleAngle(HandJoint::IndexMCP,
                                        HandJoint::IndexPIP,
                                        HandJoint::IndexDIP,
                                        CompareOp::GreaterEqual,
                                        165.0f),
                              ruleAngle(HandJoint::MiddleMCP,
                                        HandJoint::MiddlePIP,
                                        HandJoint::MiddleDIP,
                                        CompareOp::GreaterEqual,
                                        165.0f)),
                ruleDistance(HandJoint::IndexTIP, HandJoint::MiddleTIP, CompareOp::LessEqual, 4.0f)),
            2)));
    motion.append(motionTransition(
        MotionState::Moving,
        MotionState::Ready,
        10,
        ruleConsecutive(ruleDistance(HandJoint::IndexTIP, HandJoint::MiddleTIP, CompareOp::GreaterEqual, 4.8f), 10)));
    motion.append(motionTransition(MotionState::Ready, MotionState::Idle, 20, ruleHandDetect(false)));
    motion.append(motionTransition(MotionState::Moving, MotionState::Idle, 20, ruleHandDetect(false)));

    button.append(buttonTransition(
        ControlState::None,
        ControlState::LeftPressed,
        10,
        ruleConsecutive(ruleDistance(HandJoint::ThumbTIP, HandJoint::RingTIP, CompareOp::LessEqual, 3.5f), 3)));
    button.append(buttonTransition(
        ControlState::LeftPressed,
        ControlState::None,
        10,
        ruleConsecutive(ruleDistance(HandJoint::ThumbTIP, HandJoint::RingTIP, CompareOp::GreaterEqual, 5.0f), 4)));
    button.append(buttonTransition(
        ControlState::None,
        ControlState::RightPressed,
        10,
        ruleConsecutive(ruleDistance(HandJoint::ThumbTIP, HandJoint::RingPIP, CompareOp::LessEqual, 5.0f), 3)));
    button.append(buttonTransition(
        ControlState::RightPressed,
        ControlState::None,
        10,
        ruleConsecutive(ruleDistance(HandJoint::ThumbTIP, HandJoint::RingPIP, CompareOp::GreaterEqual, 5.5f), 10)));
    button.append(buttonTransition(
        ControlState::None,
        ControlState::VerticalScrolling,
        5,
        ruleConsecutive(ruleDistance(HandJoint::ThumbTIP, HandJoint::IndexTIP, CompareOp::LessEqual, 4.0f), 2)));
    button.append(buttonTransition(
        ControlState::VerticalScrolling,
        ControlState::None,
        5,
        ruleConsecutive(ruleDistance(HandJoint::ThumbTIP, HandJoint::IndexTIP, CompareOp::GreaterEqual, 4.5f), 2)));

    return QJsonObject{{"motion_transitions", motion}, {"button_transitions", button}};
}

QJsonObject buildTiktokPresetJson()
{
    QJsonArray motion;
    QJsonArray button;

    motion.append(motionTransition(
        MotionState::Idle, MotionState::Ready, 10, ruleConsecutive(ruleHandDetect(true), 3)));
    motion.append(motionTransition(
        MotionState::Ready, MotionState::Idle, 20, ruleConsecutive(ruleHandDetect(false), 5)));

    const QJsonObject thumb_index_close =
        ruleConsecutive(ruleDistance(HandJoint::ThumbTIP, HandJoint::IndexTIP, CompareOp::LessEqual, 4.0f), 2);
    const QJsonObject index_move = ruleMoveDistance(HandJoint::IndexTIP, CompareOp::GreaterEqual, 15.0f, 5);
    const QJsonObject trigger = ruleComposite(LogicOp::And, thumb_index_close, index_move);

    button.append(buttonTransition(ControlState::None, ControlState::VerticalScrolling, 10, trigger));
    button.append(buttonTransition(
        ControlState::VerticalScrolling, ControlState::None, 10, ruleConsecutive(ruleBool(true), 30)));

    return QJsonObject{{"motion_transitions", motion}, {"button_transitions", button}};
}

bool parseRuleObject(const QJsonObject& obj, RulePtr& rule)
{
    if (!obj.contains("type") || !obj.value("type").isString())
    {
        LOG(ERROR) << "Rule object must contain string field 'type'.";
        return false;
    }

    const QString type = obj.value("type").toString();

    if (type == "BoolRule")
    {
        rule = std::make_shared<Interaction::Rule::BoolRule>(obj.value("value").toBool(true));
        return true;
    }
    if (type == "HandDetectRule")
    {
        rule = std::make_shared<Interaction::Rule::HandDetectRule>(obj.value("should_detect").toBool(true));
        return true;
    }
    if (type == "DistanceRule")
    {
        HandJoint a;
        HandJoint b;
        CompareOp op;
        if (!Utils::parseHandJoint(obj.value("joint_a").toString(), a) ||
            !Utils::parseHandJoint(obj.value("joint_b").toString(), b) ||
            !Utils::parseCompareOp(obj.value("compare_op").toString(), op))
        {
            return false;
        }
        rule = std::make_shared<Interaction::Rule::DistanceRule>(
            a, b, op, static_cast<float>(obj.value("threshold").toDouble()));
        return true;
    }
    if (type == "AngleRule")
    {
        HandJoint a;
        HandJoint b;
        HandJoint c;
        CompareOp op;
        if (!Utils::parseHandJoint(obj.value("joint_a").toString(), a) ||
            !Utils::parseHandJoint(obj.value("joint_b").toString(), b) ||
            !Utils::parseHandJoint(obj.value("joint_c").toString(), c) ||
            !Utils::parseCompareOp(obj.value("compare_op").toString(), op))
        {
            return false;
        }
        rule = std::make_shared<Interaction::Rule::AngleRule>(
            a, b, c, op, static_cast<float>(obj.value("threshold_degree").toDouble()));
        return true;
    }
    if (type == "MoveDistanceRule")
    {
        HandJoint joint;
        CompareOp op;
        if (!Utils::parseHandJoint(obj.value("joint").toString(), joint) ||
            !Utils::parseCompareOp(obj.value("compare_op").toString(), op))
        {
            return false;
        }
        rule = std::make_shared<Interaction::Rule::MoveDistanceRule>(
            joint, op, static_cast<float>(obj.value("threshold").toDouble()), obj.value("frame_window").toInt(1));
        return true;
    }
    if (type == "ConsecutiveFrameRule")
    {
        if (!obj.contains("base_rule") || !obj.value("base_rule").isObject())
        {
            LOG(ERROR) << "ConsecutiveFrameRule must contain object field 'base_rule'.";
            return false;
        }
        RulePtr base;
        if (!parseRuleObject(obj.value("base_rule").toObject(), base))
        {
            return false;
        }
        rule = std::make_shared<Interaction::Rule::ConsecutiveFrameRule>(base, obj.value("required_frames").toInt(1));
        return true;
    }
    if (type == "CompositeRule")
    {
        if (!obj.contains("left_rule") || !obj.value("left_rule").isObject() || !obj.contains("right_rule") ||
            !obj.value("right_rule").isObject())
        {
            LOG(ERROR) << "CompositeRule must contain left_rule/right_rule objects.";
            return false;
        }
        LogicOp op;
        if (!Utils::parseLogicOp(obj.value("logic_op").toString(), op))
        {
            return false;
        }
        RulePtr left;
        RulePtr right;
        if (!parseRuleObject(obj.value("left_rule").toObject(), left) ||
            !parseRuleObject(obj.value("right_rule").toObject(), right))
        {
            return false;
        }
        rule = std::make_shared<Interaction::Rule::CompositeRule>(op, left, right);
        return true;
    }

    LOG(ERROR) << "Unknown rule type: " << type.toStdString();
    return false;
}

bool parseMotionTransitions(const QJsonArray& arr, std::vector<Interaction::FSM::MotionTransition>& out)
{
    out.clear();
    for (const QJsonValue& value : arr)
    {
        if (!value.isObject())
        {
            LOG(ERROR) << "Each motion transition must be an object.";
            return false;
        }

        const QJsonObject obj = value.toObject();
        MotionState from_state;
        MotionState to_state;
        if (!Utils::parseMotionState(obj.value("from").toString(), from_state) ||
            !Utils::parseMotionState(obj.value("to").toString(), to_state))
        {
            return false;
        }
        if (!obj.contains("rule") || !obj.value("rule").isObject())
        {
            LOG(ERROR) << "Motion transition must contain object field 'rule'.";
            return false;
        }
        RulePtr rule;
        if (!parseRuleObject(obj.value("rule").toObject(), rule))
        {
            return false;
        }
        out.emplace_back(from_state, to_state, rule, obj.value("priority").toInt(0));
    }
    return true;
}

bool parseButtonTransitions(const QJsonArray& arr, std::vector<Interaction::FSM::ButtonTransition>& out)
{
    out.clear();
    for (const QJsonValue& value : arr)
    {
        if (!value.isObject())
        {
            LOG(ERROR) << "Each button transition must be an object.";
            return false;
        }

        const QJsonObject obj = value.toObject();
        ControlState from_state;
        ControlState to_state;
        if (!Utils::parseControlState(obj.value("from").toString(), from_state) ||
            !Utils::parseControlState(obj.value("to").toString(), to_state))
        {
            return false;
        }
        if (!obj.contains("rule") || !obj.value("rule").isObject())
        {
            LOG(ERROR) << "Button transition must contain object field 'rule'.";
            return false;
        }
        RulePtr rule;
        if (!parseRuleObject(obj.value("rule").toObject(), rule))
        {
            return false;
        }
        out.emplace_back(from_state, to_state, rule, obj.value("priority").toInt(0));
    }
    return true;
}

QJsonObject createPresetJson(ControlPreset preset)
{
    switch (preset)
    {
        case ControlPreset::Easy: return buildEasyPresetJson();
        case ControlPreset::Advanced: return buildAdvancedPresetJson();
        case ControlPreset::Tiktok: return buildTiktokPresetJson();
        default: return buildEasyPresetJson();
    }
}
} // namespace

QString FsmConfig::defaultPath()
{
    return QString::fromStdString(Paths::fsmConfigDir());
}

QString FsmConfig::presetFilePath(const QString& preset_name)
{
    return defaultPath() + "/" + preset_name + ".json";
}

bool FsmConfig::ensureBuiltinPresets(bool overwrite)
{
    const bool ok = generatePreset("easy", ControlPreset::Easy, overwrite) &&
                    generatePreset("advanced", ControlPreset::Advanced, overwrite) &&
                    generatePreset("tiktok", ControlPreset::Tiktok, overwrite);
    if (ok)
    {
        LOG(INFO) << "Built-in FSM presets are ready.";
    }
    return ok;
}

bool FsmConfig::generatePreset(const QString& preset_name, ControlPreset preset, bool overwrite)
{
    const QString file_path = presetFilePath(preset_name);
    const QFileInfo file_info(file_path);
    if (file_info.exists() && !overwrite)
    {
        return true;
    }
    return saveJsonFile(file_path, createPresetJson(preset));
}

bool FsmConfig::load(Interaction::FSM::RuleSet& rules, const QString& preset_name)
{
    QJsonObject root_obj;
    if (!loadJsonFile(presetFilePath(preset_name), root_obj))
    {
        return false;
    }

    std::vector<Interaction::FSM::MotionTransition> motion_transitions;
    std::vector<Interaction::FSM::ButtonTransition> button_transitions;

    if (root_obj.contains("motion_transitions"))
    {
        if (!root_obj.value("motion_transitions").isArray())
        {
            LOG(ERROR) << "motion_transitions must be an array.";
            return false;
        }
        if (!parseMotionTransitions(root_obj.value("motion_transitions").toArray(), motion_transitions))
        {
            return false;
        }
    }

    if (root_obj.contains("button_transitions"))
    {
        if (!root_obj.value("button_transitions").isArray())
        {
            LOG(ERROR) << "button_transitions must be an array.";
            return false;
        }
        if (!parseButtonTransitions(root_obj.value("button_transitions").toArray(), button_transitions))
        {
            return false;
        }
    }

    rules.motion_transitions = std::move(motion_transitions);
    rules.button_transitions = std::move(button_transitions);
    LOG(INFO) << "FSM preset loaded: " << preset_name.toStdString();
    return true;
}

} // namespace Config
} // namespace VisionCursor
