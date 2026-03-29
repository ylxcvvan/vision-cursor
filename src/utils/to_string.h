#pragma once

#include <QString>

#include "common/common.h"

namespace VisionCursor
{
namespace Utils
{

QString ToString(MotionState state);
QString ToString(ControlState state);
QString ToString(HandJoint joint);
QString ToString(CompareOp op);
QString ToString(LogicOp op);
QString ToString(MoveMode mode);
QString ToString(ControlPreset preset);

bool parseMotionState(const QString& str, MotionState& state);
bool parseHandJoint(const QString& str, HandJoint& joint);
bool parseControlState(const QString& str, ControlState& state);
bool parseCompareOp(const QString& str, CompareOp& op);
bool parseLogicOp(const QString& str, LogicOp& op);
bool parseMoveMode(const QString& str, MoveMode& mode);
bool parseControlPreset(const QString& str, ControlPreset& preset);

} // namespace Utils
} // namespace VisionCursor