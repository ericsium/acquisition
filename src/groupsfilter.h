/*
    Copyright 2014 Ilya Zhuravlev

    This file is part of Acquisition.

    Acquisition is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Acquisition is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Acquisition.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "filters.h"
#include "modsfilter.h"

#include <QGridLayout>
#include <QObject>
#include <QSignalMapper>
#include <vector>
#include <QComboBox>
#include <QCompleter>
#include <QLineEdit>
#include <QPushButton>
#include <deps/better-enums/enum.h>

BETTER_ENUM(GroupType, char, And=1, Not, Count, Sum, If)

class SelectedGroup {
public:
    SelectedGroup(const std::string &name, double min, double max, bool min_selected, bool max_selected);
    SelectedGroup(const SelectedGroup&) = delete;
    SelectedGroup& operator=(const SelectedGroup&) = delete;
    SelectedGroup(SelectedGroup&& o) = default;
    SelectedGroup& operator=(SelectedGroup&& o) = default;

    void Update();
    void AddToLayout(QVBoxLayout *layout, int index);
    void CreateSignalMappings(QSignalMapper *signal_mapper, int index);
    void RemoveSignalMappings(QSignalMapper *signal_mapper);
    void SetIndex(size_t index) { index_ = index; }
    const ModFilterData &data() const { return data_; }
    size_t index() const { assert(index_<999); return index_; }
    std::unique_ptr<ModsFilter> mods_filter_;
private:
    ModFilterData data_;
    std::unique_ptr<QComboBox> group_select_;
    QCompleter *group_completer_;
    std::unique_ptr<QLineEdit> min_text_, max_text_;
    std::unique_ptr<QPushButton> delete_button_;
    std::unique_ptr<QVBoxLayout> mods_layout_;
    std::unique_ptr<QHBoxLayout> group_layout_;
    size_t index_{999};
};

class GroupsFilter;

class GroupsFilterSignalHandler : public QObject {
    Q_OBJECT
public:
    GroupsFilterSignalHandler(GroupsFilter &parent) :
        parent_(parent)
    {}
signals:
    void SearchFormChanged();
private slots:
    void OnAddButtonClicked();
    void OnGroupChanged(int id);
private:
    GroupsFilter &parent_;
};

class GroupsFilter : public Filter, public QObject {
    Q_GADGET
    friend class GroupsFilterSignalHandler;
public:
    enum Type {
        And,
        Not,
        Count,
        Sum,
        If
    };
    Q_ENUM(Type);

    explicit GroupsFilter(QLayout *parent);
    void FromForm(FilterData *data);
    void ToForm(FilterData *data);
    void ResetForm();
    bool Matches(const std::shared_ptr<Item> &item, FilterData *data);
private:
    void Clear();
    void ClearSignalMapper();
    void ClearLayout();
    void Initialize(QLayout *parent);
    void Refill();
    void AddGroup();
    void UpdateGroup(int id);
    void DeleteGroup(int id);

    std::unique_ptr<QVBoxLayout> layout_;
    std::unique_ptr<QPushButton> add_button_;
    std::vector<SelectedGroup> groups_;
    GroupsFilterSignalHandler signal_handler_;
    QSignalMapper signal_mapper_;
};
