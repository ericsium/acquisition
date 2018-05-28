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


class SelectedGroup {
public:
    SelectedGroup(const std::string &name, double min, double max, bool min_selected, bool max_selected);
    SelectedGroup(const SelectedGroup&) = delete;
    SelectedGroup& operator=(const SelectedGroup&) = delete;
    SelectedGroup(SelectedGroup&& o) :
        data_(std::move(o.data_)),
        group_select_(std::move(o.group_select_)),
        min_text_(std::move(o.min_text_)),
        max_text_(std::move(o.max_text_)),
        delete_button_(std::move(o.delete_button_)),
        mods_filter_(std::move(o.mods_filter_))
    {}
    SelectedGroup& operator=(SelectedGroup&& o) {
        data_ = std::move(o.data_);
        group_select_ = std::move(o.group_select_);
        min_text_ = std::move(o.min_text_);
        max_text_ = std::move(o.max_text_);
        delete_button_ = std::move(o.delete_button_);
        mods_filter_ = std::move(o.mods_filter_);
        return *this;
    }

    void Update();
    void AddToLayout(QGridLayout *layout, int index);
    void CreateSignalMappings(QSignalMapper *signal_mapper, int index);
    void RemoveSignalMappings(QSignalMapper *signal_mapper);
    const ModFilterData &data() const { return data_; }
private:
    ModFilterData data_;
    std::unique_ptr<QComboBox> group_select_;
    QCompleter *group_completer_;
    std::unique_ptr<QLineEdit> min_text_, max_text_;
    std::unique_ptr<QPushButton> delete_button_;
    std::unique_ptr<ModsFilter> mods_filter_;
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
    void OnModChanged(int id);
private:
    GroupsFilter &parent_;
};

class GroupsFilter : public Filter {
    friend class GroupsFilterSignalHandler;
public:
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
    void AddMod();
    void UpdateMod(int id);
    void DeleteMod(int id);

    std::unique_ptr<QGridLayout> layout_;
    std::unique_ptr<QPushButton> add_button_;
    std::vector<SelectedGroup> groups_;
    GroupsFilterSignalHandler signal_handler_;
    QSignalMapper signal_mapper_;
};
