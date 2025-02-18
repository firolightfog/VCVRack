#ifndef EXTENDED_TEXT_H
#define EXTENDED_TEXT_H

#include <algorithm>
#include <string>
#include <vector>
/*
Library that helps my subclasses of LedDisplayTextField do useful
features like being longer than the screen and support up/down keys.
*/

struct TextLine {
  int line_number;
  int start_position;
  int line_length;

  TextLine(int line, int position, int length) : line_number{line},
      start_position{position}, line_length{length} {}
  bool operator==(const TextLine a) {
    return (a.line_number == line_number) &&
           (a.start_position == start_position) &&
           (a.line_length == line_length);
  }
};

struct LineColumn {
  int line;
  int column;

  LineColumn(int line, int column) : line{line}, column{column} {}
  bool operator==(const LineColumn a) {
    return (a.line == line) && (a.column == column);
  }
};

struct ExtendedText {
  // Sorted searchable map from position -> line+column.
  std::vector<TextLine> line_map;
  // Number of lines in the visible area.
  int window_length;
  // Number of lines away from the edge before we start scrolling.
  // Zero may not be a value that I can get to work.
  int buffer_length;
  // Track how many lines are above the visible window.
  // -1 signals that we are ready to completely reassign the window.
  // For example, at startup.
  int lines_above = -1;

  void Initialize(int length, int buffer) {
    window_length = length;
    buffer_length = buffer;
  }

  // The position that the window shows is always relative to the text *in
  // the window*, and not the whole text. So there are places we need to
  // surface that.
  int CharsAbove() {
    if (lines_above < 0) {
      return 0;
    } else {
      if (lines_above < (int) line_map.size()) {
        return line_map[lines_above].start_position;
      } else {
        // Not sure what's happening at this point.
        return 0;
      }
    }
  }

  void ProcessUpdatedText(const std::string &text) {
    // Clear and repopulate line_map.
    line_map.clear();
    int line_number = 0;
    // 'pos <= text.size()' is correct; the last line might be an empty line
    // directly after a \n.
    for (size_t pos = 0;  pos <= text.size(); ) {
      size_t found = text.find('\n', pos);
      if (found != std::string::npos) {
        TextLine tl(line_number, pos, found - pos);
        line_map.push_back(tl);
        pos = found + 1;
        line_number++;
      } else {
        // Add the last line.
        TextLine tl(line_number, pos, text.size() - pos);
        line_map.push_back(tl);
        break;
      }
    }
  }

  LineColumn GetCurrentLineColumn(int position) {
    int line_number;
    auto next_line = std::find_if(line_map.begin(), line_map.end(),
        [position](TextLine tl) { return position < tl.start_position; });
    if (next_line == line_map.end()) {
      // We're on the last line.
      line_number = line_map.size() - 1;
    } else {
      line_number = next_line->line_number - 1;
    }
    return LineColumn(line_number,
        position - line_map[line_number].start_position);
  }

  // Before calling this, line_map *must* be accurate to the current state of
  // 'text'.
  // Updates lines_above.
  void RepositionWindow(int current_full_position) {
    // Which line are we currently on?
    LineColumn lc = GetCurrentLineColumn(current_full_position);
    // First, figure out what we want.
    // For this to really work, we need to know what the top line
    // of the existing window text is. That's what lines_above is for.
    // lines_above also gives us a sense of which direction user came from to
    // get to current_position.
    int top_line;
    if (lines_above < 0) {
      // I now think this only happens when program is first loaded,
      // and we are already looking at the top.
      top_line = 0;
    } else {
      if (lc.line >= lines_above + window_length - buffer_length) {
        // We are below visible text, move down.
        top_line = lc.line - window_length + buffer_length + 1;
      } else if (lc.line <= lines_above + buffer_length) {
        // We are getting close to the top, move so position is on the top line.
        top_line = std::max(0, lc.line - buffer_length);
      } else {
        // Position is currently visible, do not change the visible text.
        return;
      }
    }
    lines_above = top_line;
    return;
  }

  int GetCursorForLineColumn(int line, int column) {
    // Enforce line number limits.
    if (line < 0) {
      line = 0;
    }
    if (line >= (int) line_map.size()) {
      line = line_map.size() - 1;
    }
    TextLine tl = line_map[line];
    if (tl.line_length < column) {
      column = tl.line_length;
    }
    return tl.start_position + column;
  }
};

#endif // EXTENDED_TEXT_H
