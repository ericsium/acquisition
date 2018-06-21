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

#include "groupsFilter.h"
#include "modsfilter.h"
#include <QComboBox>
#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include "QsLog.h"
#include <QVBoxLayout>

#include "porting.h"
#include "util.h"

SelectedGroup::SelectedGroup(const std::string &name, double min, double max, bool min_filled, bool max_filled) :
    data_(name, min, max, min_filled, max_filled),
    group_select_(std::make_unique<QComboBox>()),
    min_text_(std::make_unique<QLineEdit>()),
    max_text_(std::make_unique<QLineEdit>()),
    delete_button_(std::make_unique<QPushButton>("x"))
{
    group_layout_ = std::make_unique<QHBoxLayout>();
    mods_layout_ = std::make_unique<QVBoxLayout>();
    group_layout_->setContentsMargins(0, 0, 0, 0);
    mods_layout_->setContentsMargins(0, 0, 0, 0);

    QStringList group_type_list;
    for (auto &name : GroupType::_names()) {
        group_type_list.push_back(name);
    }
    group_select_->setEditable(false);
    group_select_->addItems(group_type_list);
    group_completer_ = new QCompleter(group_type_list);
    group_completer_->setCompletionMode(QCompleter::PopupCompletion);
    group_completer_->setFilterMode(Qt::MatchContains);
    group_completer_->setCaseSensitivity(Qt::CaseInsensitive);
    group_select_->setCompleter(group_completer_);

    group_select_->setCurrentIndex(group_select_->findText(name.c_str()));
    if (min_filled)
        min_text_->setText(QString::number(min));
    if (max_filled)
        max_text_->setText(QString::number(max));
}

void SelectedGroup::Update() {
    data_.mod = group_select_->currentText().toStdString();
    data_.min = min_text_->text().toDouble();
    data_.max = max_text_->text().toDouble();
    data_.min_filled = !min_text_->text().isEmpty();
    data_.max_filled = !max_text_->text().isEmpty();
}

void SelectedGroup::AddToLayout(QVBoxLayout *layout, int index) {
    group_layout_->addWidget(group_select_.get());
    group_layout_->addWidget(min_text_.get());
    group_layout_->addWidget(max_text_.get());
    group_layout_->addWidget(delete_button_.get());

    QLOG_INFO() << "AddLayout1" << group_layout_->count();
    QLOG_INFO() << "AddLayout2" << layout->count();
    if (!group_layout_->parent()) {
        layout->addLayout(group_layout_.get());
    }

    QLOG_INFO() << "AddLayout2";
    if (!mods_layout_->parent()) {
        layout->addLayout(mods_layout_.get());
    }

    if (!mods_filter_) {
        mods_filter_ = std::make_unique<ModsFilter>(mods_layout_.get());
    }
}

void SelectedGroup::CreateSignalMappings(QSignalMapper *signal_mapper, int index) {
    QObject::connect(group_select_.get(), SIGNAL(currentIndexChanged(int)), signal_mapper, SLOT(map()));
    QObject::connect(min_text_.get(), SIGNAL(textEdited(const QString&)), signal_mapper, SLOT(map()));
    QObject::connect(max_text_.get(), SIGNAL(textEdited(const QString&)), signal_mapper, SLOT(map()));
    QObject::connect(delete_button_.get(), SIGNAL(clicked()), signal_mapper, SLOT(map()));

    signal_mapper->setMapping(group_select_.get(), index + 1);
    signal_mapper->setMapping(min_text_.get(), index + 1);
    signal_mapper->setMapping(max_text_.get(), index + 1);
    // hack to distinguish update and delete, delete event has negative (id + 1)
    signal_mapper->setMapping(delete_button_.get(), -index - 1);
}

void SelectedGroup::RemoveSignalMappings(QSignalMapper *signal_mapper) {
    signal_mapper->removeMappings(group_select_.get());
    signal_mapper->removeMappings(min_text_.get());
    signal_mapper->removeMappings(max_text_.get());
    signal_mapper->removeMappings(delete_button_.get());
}

GroupsFilter::GroupsFilter(QLayout *parent):
    signal_handler_(*this)
{
    Initialize(parent);
    QObject::connect(&signal_handler_, SIGNAL(SearchFormChanged()), 
        parent->parentWidget()->window(), SLOT(OnDelayedSearchFormChange()));
}

void GroupsFilter::FromForm(FilterData *data) {
    auto &group_data = data->group_data;
    group_data.clear();
    for (auto &group : groups_) {
        group_data.push_back({group.data(), {}});
        group.mods_filter_->FromForm(data, group.index());
    }
}

void GroupsFilter::ToForm(FilterData *data) {
    Clear();
    for (auto const & pair: data->group_data) {
        auto const & group = pair.first;
        groups_.push_back(SelectedGroup(group.mod, group.min, group.max, group.min_filled, group.max_filled));
    }

    Refill();
    // Must refill once to create layouts before forcing ToForm which requires them
    for (auto &group : groups_) {
        group.mods_filter_->ToForm(data, group.index());
    }
}

void GroupsFilter::ResetForm() {
    Clear();
    Refill();
    for (auto &group : groups_) {
        group.mods_filter_->ResetForm();
    }
}

bool GroupsFilter::Matches(const std::shared_ptr<Item> &item, FilterData *data) {
    return std::all_of(groups_.begin(), groups_.end(), [&item, data](auto const &group) {
        //auto const &mods = data->group_data[group.index()].second;
        return group.mods_filter_->Matches(item, data, group.index());
    });
}

void GroupsFilter::Initialize(QLayout *parent) {
    layout_ = std::make_unique<QVBoxLayout>();
    add_button_ = std::make_unique<QPushButton>("Add group");
    QObject::connect(add_button_.get(), SIGNAL(clicked()), &signal_handler_, SLOT(OnAddButtonClicked()));
    Refill();

    auto widget = new QWidget;
    widget->setContentsMargins(0, 0, 0, 0);
    widget->setLayout(layout_.get());
    parent->addWidget(widget);

    QObject::connect(&signal_mapper_, SIGNAL(mapped(int)), &signal_handler_, SLOT(OnGroupChanged(int)));
}

void GroupsFilter::AddGroup() {
    SelectedGroup group("And", 0, 0, false, false);
    groups_.push_back(std::move(group));
    Refill();
}

void GroupsFilter::UpdateGroup(int id) {
    groups_[id].Update();
}

void GroupsFilter::DeleteGroup(int id) {
    groups_.erase(groups_.begin() + id);

    Refill();
}

void GroupsFilter::ClearSignalMapper() {
    for (auto &mod : groups_) {
        mod.RemoveSignalMappings(&signal_mapper_);
    }
}

void GroupsFilter::ClearLayout() {
    QLOG_INFO() << "GroupsFilter::ClearLayout";
    //QLayoutItem *item;
    //while ((item = layout_->takeAt(0))) {}

    //Util::ClearLayout(layout_.get());
}

void GroupsFilter::Clear() {
    ClearSignalMapper();
    ClearLayout();
    groups_.clear();
}

void GroupsFilter::Refill() {
    ClearSignalMapper();
    ClearLayout();
    int i = 0;
    for (auto &group : groups_) {
        group.AddToLayout(layout_.get(), i);
        group.CreateSignalMappings(&signal_mapper_, i);
        group.SetIndex(i);
        ++i;
    }
    layout_->addWidget(add_button_.get());
}

void GroupsFilterSignalHandler::OnAddButtonClicked() {
    parent_.AddGroup();
}

void GroupsFilterSignalHandler::OnGroupChanged(int id) {
    if (id < 0)
        parent_.DeleteGroup(-id - 1);
    else
        parent_.UpdateGroup(id - 1);
    emit SearchFormChanged();
}
