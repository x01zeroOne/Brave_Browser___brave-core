Note:
* May not work well with upstream's tab strip flags
* Per DPI setting, there could be 1px off - It'd be great if we can find them.
* @annotations for denoting what context should be considered together


Feature: Tab strip orientation change

    We support to switching tab strip's orientation at run time.

    Scenario: Flag should be enabled by default on Beta (NOT MERGED YET)
        When I open a context menu on tabs
        Then the context menu should have "Use vertical tabs" item

    @should-be-tested-with-groups
    @should-be-tested-with-pinned-tabs
    @should-be-tested-with-multiple-windows
    @should-be-tested-with-incognito-windows
    Scenario: Switching from horizontal tab strip to vertical tab strip and vice versa.
        When I click context menu item "Use vertical tabs"
        Then the tabstrip's orientation should be changed



Feature: Vertical tab strip's visual mode

    We have 3 visual mode for: Minimized, Expanded, and Floating

    @should-be-tested-with-pinned-tabs
    Scenario: Using vertical tab strip expanded
    When I have multiple pinned tabs and unpinned tabs
    Then pinned tabs laid out in grid
    * unpinned tabs are laid out vertically

    @should-be-tested-with-all-vertical-tab-strip-visual-mode
    Scenario: Using overflowed vertical tab strip
    When I have so many tabs that tabs' height is taller than vertical tab strip
    Then the tab strip should be scrollable

    Scenario: Using overflowed vertical tab strip
    When I have so many tabs that tabs' height is taller than vertical tab strip
    Then the tab strip should be scrollable


    Scenario: Activating a tab while tab strip is scrolled
    Given tabs are overflowed and tab strip is scrollable
    When create a new tab
    Then tab strip should scroll to the point the new tab was created at.

    Given scroll position is first / last
    When Activate the last / first tab with shorcut (Control + PgDw/PgUp)
    Then tab strip should scroll to the position the new active is at.


    @should-be-tested-with-groups
    @should-be-tested-with-pinned-tabs
    @should-be-tested-with-multiple-windows
    @should-be-tested-with-incognito-windows
    Scenario: Minimizing vertical tab strip
        Given Vertical tab strip is in full size, which is initial state,
        When I click a toggle button on the left side of vertical tab strip header area
        Then Vertical tab strip should be minimized
        * Only favicon should be visible
        * Tab group header is also minimized
        * Regardless pinned state of tabs, they should be in a column.
        * All the vertical tabs of windows should be minimized together

    @should-be-tested-with-pinned-tabs
    Scenario: Entering/Exiting Floating mode
        Given Vertical tab stirp is minimized
        When I move mouse over the vertical tab strip area
        Then vertical tab strip should be expanded
        * Regardless pinned state of tabs, they should be in a column.
        * Tabs should be aligned to the left edge, and only right side should be extended, as long as possible.
        * Vertical tab strip overlays other UI to the right, such as web page, side bar

        When I move mouse out of vertical tab strip area,
        Then vertical tab should be minimized

    Scenario: When bookmark bar is visible only on NTP
        Given the value of `brave://settings/appearance > Show bookmarks` is "only on new tab page"
        When the active tab is NTP,
        Then bookmark bar should be next to vertical tab strip, not above it.

    Scenario: Floating mode when the bookmark bar is visible only on NTP
        Given the value of `brave://settings/appearance > Show bookmarks` is "only on new tab page"
        When the active tab is NTP,
        And vertical tab strip enters floating,
        Then vertical tab strip should overlay bookmark bar.
        * bookmarks bar's position shouldn't change at all.

    @should-be-tested-with-groups
    @should-be-tested-with-pinned-tabs
    @should-be-tested-with-multiple-windows
    @should-be-tested-with-incognito-windows
    Scenario: Expanding vertical tab strip
        Given Vertical tab stirp isn't exapnded, i.e. minimized or floating
        When I click a toggle button on the left side of vertical tab strip header area
        Then vertical tab strip should be widen.
        * Other UI will be pushed to the right
        * All vertical tabs of windows should be expanded together

    @should-be-tested-with-incognito-windows
    Scenario: En/Disabling floating mode
        Given vertical tab strip is being used
        When I open a context menu on a tab
        And Click "Float on mouseover"
        Then The checkbox state should be toggled
        * According to the checked state, floating mode should be en/disabled

    @should-be-tested-with-multiple-windows
    @should-be-tested-with-incognito-windows
    Scenario: Showing/Hiding window title bar
        Given vertical tab strip is being used
        When I open a context menu on a tab
        And Click "Show title bar"
        Then The checkbox state should be toggled
        * According to the checked state, Window title bar should be visible/hidden



Feature: Fullscreen and vertical tab strip visibility
        There're two types of fullscreen mode.
        * browser-wide: via F11(Windows) or a button on frame(Mac)
        * from contents: video fullscreen

    Scenario: Browser-wide fullscreen
        When I enter browser-wide fullscreen
        Then the vertical tab strip should be visible
        * All features should work as well

    Scenario: Fullscreen from contents
        When I enter fullscreen for contents
        Then the vertical tab strip shouldn't be visible



Feature: Basic tab operations

    Basic operations such as creating, closing, and etc should work when
    using vertical tab strip

    Scenario: Opening a new tab
        When I click on the "New Tab" button
        Then a new tab should open
        And the new tab should be active
        And the URL of the new tab should be the default home page

        When I press {Ctrl, Cmd, Meta} + T
        Then a new tab should open.(same as above.)

    Scenario: Navigating to a website in a tab
        When I enter "https://www.example.com" in the URL bar of the new tab
        Then the website "https://www.example.com" should load in the new tab
        And the title and favicon(loading spinner) of the new tab should match the loaded site

    Scenario: Switching between tabs
        When I click on a different tab
        Then the clicked tab should become active
        * the previously active tab should be inactive,
        * it should be visually distinguishable.

    @should-be-tested-with-multiple-tabs-selected
    Scenario: Closing a tab
        When I click on the "Close" button of a tab
        Then the tab should be closed
        * the previously active tab should become active
        * the closed tab should no longer be visible

        When I press {Ctrl, Cmd, Meta} + w
        Then the tab should be closed. (same as above.)

    @should-be-tested-with-multiple-tabs-selected
    Scenario: Pinning and unpinning a tab
        When I right-click on the tab
        And I select "Pin" from the context menu
        Then the tab should be pinned
        * the tab should be smaller in size
        * the all pinned tabs are laid out above unpinned tabs
        * the tab should be positioned on the left side of the tab bar

        When I right-click on the pinned tab
        And I select "Unpin" from the context menu
        Then the tab should be unpinned
        * the tab should return to its normal size
        * the tab should no longer be pinned
        * the title of the tab should be fully visible
        * the tab should be positioned according to its position before pinning

    @should-be-tested-with-multiple-tabs-selected
    Scenario: Muting and unmuting a tab
        Given I openend a tab with a video and play the video
        When I click the "Mute Tab" item from the tab's context menu
        Then the tab should be muted
        * And the speaker button should be toggled with dash.

        Given I muted a tab, while playing a video
        When I click the "Unmute tab" item from the tab's context menu
        Then the tab should be unmuted
        * And the speaker button should be visible

        Given I have a tab playing a video
        When I click the speaker button
        Then the muting state should be toggled

    @should-be-tested-with-multiple-tabs-selected
    Scenario: Duplicating tab(s)
        When I right-click on the tab
        And I select "Duplicate" from the context menu
        Then new tab(s) with the same website should open
        And the new tab should be active
        And the content of the new tab should be the same as the original tab

    Scenario: Using tab search feature
        Given vertical tab strip header has "Tab search" button on the right
        (On windows 10 we have the button on the frame)
        When I click the "Tab search" button
        Then "Tab search bubble" should show up to the right side of the button

        @should-be-tested-with-all-vertical-tab-strip-visual-mode
        When I press {Ctrl, Command} + Shift + A
        Then "Tab search bubble" should show up to the right side of the button

    
    Scenario: Using tab hover feature
        When I move mouse over a tab
        Then tab hover card should show up
        * It should be on the right of the tab


    Scenario: Enabling Vertical tabs with Sidebar
        Given I am using sidebar
        And I have never changed the position of Sidebar - on the left
        When I enable vertical tab strip
        Then the side bar should move to the right side of browser

        Given I have changed sidebar's position
        When I enable vertical tap bstrip
        Then the side bar will be next to the vertical tab strip.

        Given vertical tab strip is enable
        When I change sidebar's position to the left,
        Then the sidebar will be next to the vertical tab strip



Feature: Tab Grouping

    Vertical tab strip supports tab groups. Group headers and group lines will
    be laid out vertically.

    @should-be-tested-with-multiple-tabs-selected
    Scenario: Creating a tab group
        When I right-click on a tab
        And I select "Add tab group" from the context menu
        Then a new tab group should be created
        And the selected tab should be added to the new group

    @should-be-tested-with-multiple-tabs-selected
    Scenario: Moving tabs between tab groups
        Given multiple tab groups are created
        When I drag and drop a tab from one group to another
        Then the tab should be moved from the source group to the target group
        And the tab should be positioned at the end of the target group's tabs

    Scenario: Collapsing and expanding tab groups
        Given multiple tab groups are created
        When I click a tab group header
        Then the tab group should collapse/expand
        And the tabs within the collapsed group should be hidden/visible

    Scenario: Closing a tab group
        When I right-click on the tab group
        And I select "Close group" from the context menu
        Then all tabs within the tab group should be closed
        And the tab group should be removed from the tab strip

    Scenario: Ungrouping a tab group
        When I right-click on the tab group
        And I select "Ungroup" from the context menu
        Then all tabs within the tab group should be ungrouped
        And the tab group should be removed from the tab strip

    Scenario: Opening a tab group into new Window
        When I right-click on the tab group
        And I select "Move Group to new window" from the context menu
        Then a new window should open and it should have the group
        * The new window also has vertical tab strip



Feature: Drag and drop

    Users can reorder tabs and groups or manage windows with drag and drop.

    @should-be-tested-with-all-vertical-tab-strip-visual-mode
    Scenario: Dragging the only tab in a window
        Given there's only one tab in a window
        When I click and hold on a tab
        And I drag the tab to a new position
        Then the window should move

    @should-be-tested-with-multiple-tabs-selected
    @should-be-tested-with-maximized-window
    @should-be-tested-with-all-vertical-tab-strip-visual-mode
    Scenario: Dragging tab(s) within the same window
        Given multiple tabs are open
        When I click and hold on tab(s)
        And I drag the tab to a new position within the same window
        Then the tab should be moved to the new position
        * the tab order should be updated accordingly

    @should-be-tested-with-multiple-tabs-selected
    @should-be-tested-with-maximized-window
    @should-be-tested-with-all-vertical-tab-strip-visual-mode
    Scenario: Dragging tab(s) to a new window
        When I click and hold on tab(s)
        And I drag the tab out of the current window
        Then a new window should be created
        * the tab should be moved to the new window
        * the tab should be positioned as the first tab in the new window

    @should-be-tested-with-multiple-tabs-selected
    @should-be-tested-with-maximized-window
    @should-be-tested-with-all-vertical-tab-strip-visual-mode
    Scenario: Dragging tab(s) to an existing window
        Given multiple windows are open
        When I click and hold on tab(s)
        And I drag the tab to an existing window
        And I drop the tab into the existing window
        Then the tab should be moved to the existing window
        * the tab should be positioned as the last tab in the window

    @should-be-tested-with-multiple-tabs-selected
    @should-be-tested-with-maximized-window
    @should-be-tested-with-all-vertical-tab-strip-visual-mode
    Scenario: Dragging tab(s) to a tab group
        Given multiple tab groups are created
        When I click and hold on tab(s)
        And I drag the tab to a tab group
        And I drop the tab into the tab group
        Then the tab should be moved to the tab group
        * the tab should be positioned as the last tab in the tab group


    @should-be-tested-with-maximized-window
    @should-be-tested-with-all-vertical-tab-strip-visual-mode
    Scenario: Dragging tab group header
        Given multiple tabs are open
        When I click and hold on tab(s)
        And I drag the tab to a new position within the same window
        Then the tab should be moved to the new position
        * the tab order should be updated accordingly


    Scenario: Drag and drop with minimized vertical tab strip
        Given vertical tab strip is minimized and floating mode is disabled
        And there're multiple windows
        When I drag a tab from a window to another window
        Then the window that is drop target should become floating mode temporarily.

        When Drag and drop session ends or dragging a tab out of tab strip
        Then the floating mode should finish


