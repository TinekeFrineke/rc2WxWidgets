#pragma once

#include <iosfwd>
#include <optional>
#include <string>

struct RcDialog;
struct RcControl;

class RcControlParser
{
public:
	explicit RcControlParser(RcDialog& targetDialog);

	void Parse(std::istream& input);

private:
	std::optional<RcControl> parseControlLine(const std::string& line);

	RcDialog& m_targetDialog;
};