#ifndef LIB_ACTIONS_ACTION_HPP
#define LIB_ACTIONS_ACTION_HPP

enum ActionType {
    HIGHLIGHT_TYPE = 100
};

struct HighlightAction {
    ActionType type = HIGHLIGHT_TYPE;
    bool highlight = false;

    HighlightAction(bool highlight = false) : highlight(highlight) {}
};

union Action {
    ActionType type;
    HighlightAction highlight;
};

#endif