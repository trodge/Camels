#include "pager.hpp"

void Pager::setVisible() {
    // Return a range of iterators for the boxes on the current page, or all boxes if there are no pages.
    size_t pageCount = indices.size();
    visible = {boxes.begin() + (pageCount ? *pageIt : 0),
               pageCount > 1u && pageIt != indices.end() - 1 ? boxes.begin() + *(pageIt + 1) : boxes.end()};
}

TextBox *Pager::getVisible(size_t idx) {
    // Get pointer to box with given index currently visible on this pager.
    return (visible.start + idx)->get();
}

std::vector<TextBox *> Pager::getVisible() {
    // Get vector of pointers to all boxes currently visible on this pager.
    std::vector<TextBox *> bxs;
    std::transform(visible.start, visible.stop, std::back_inserter(bxs),
                   [](std::unique_ptr<TextBox> &bx) { return bx.get(); });
    return bxs;
}

std::vector<TextBox *> Pager::getAll() {
    std::vector<TextBox *> bxs;
    std::transform(boxes.begin(), boxes.end(), std::back_inserter(bxs),
                   [](std::unique_ptr<TextBox> &bx) { return bx.get(); });
    return bxs;
}

int Pager::visibleCount() const { return visible.stop - visible.start; }

void Pager::addBox(std::unique_ptr<TextBox> &&bx) {
    boxes.push_back(std::move(bx));
    setVisible();
}

void Pager::addPage(std::vector<std::unique_ptr<TextBox>> &bxs) {
    // Move parameter boxes and pagers into member vectors giving them a new page. Sets page to that page.
    size_t boxCount = boxes.size();
    indices.push_back(boxCount);
    pageIt = indices.end() - 1;
    boxCount += bxs.size();
    boxes.reserve(boxCount);
    std::move(bxs.begin(), bxs.end(), std::back_inserter(boxes));
    setVisible();
    bxs.clear();
}

void Pager::advancePage() {
    // Advance to next page. Prevent advancing past last page.
    if (pageIt != indices.end() - 1) ++pageIt;
    setVisible();
}

void Pager::recedePage() {
    // Recede to previous page. Prevent receding past first page.
    if (pageIt != indices.begin() + 1) --pageIt;
    setVisible();
}

int Pager::getKeyedIndex(const SDL_KeyboardEvent &k) {
    // Return the index relative to current page of keyed box, or -1 if no box was keyed on current page.
    auto keyedIt =
        std::find_if(visible.start, visible.stop, [&k](const std::unique_ptr<TextBox> &bx) { return bx->keyCaptured(k); });
    return keyedIt == visible.stop ? -1 : keyedIt - visible.start;
}

int Pager::getClickedIndex(const SDL_MouseButtonEvent &b) {
    // Return the index relative to current page of clicked box, or -1 if no box was clicked on current page.
    auto clickedIt = std::find_if(visible.start, visible.stop,
                                  [&b](const std::unique_ptr<TextBox> &bx) { return bx->clickCaptured(b); });
    return clickedIt == visible.stop ? -1 : clickedIt - visible.start;
}

void Pager::draw(SDL_Renderer *s) {
    // Draw all boxes on the current page, or all boxes if there are no pages.
    for (auto bxIt = visible.start; bxIt != visible.stop; ++bxIt) (*bxIt)->draw(s);
}
