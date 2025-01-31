// Symphony of Empires
// Copyright (C) 2021, Symphony of Empires contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------
// Name:
//      client/interface/policies.cpp
//
// Abstract:
//      Does some important stuff.
// ----------------------------------------------------------------------------

#include "eng3d/serializer.hpp"
#include "eng3d/ui/components.hpp"
#include "eng3d/texture.hpp"
#include "client/interface/policies.hpp"
#include "client/client_network.hpp"
#include "action.hpp"

using namespace Interface;

#define POLICY_CHECKBOX(x, title, body)\
    auto* x## _chk = new UI::Checkbox(0, 0, 128, 24, reform_grp);\
    x## _chk->text(title);\
    x## _chk->set_on_click([this](UI::Widget& w) {\
        this->new_policy.x = !(this->new_policy).x;\
        ((UI::Checkbox&)w).set_value((this->new_policy).x);\
    });\
    x## _chk->set_value(new_policy.x);\
    x## _chk->set_tooltip(body);

PoliciesScreen::PoliciesScreen(GameState& _gs)
    : UI::Window(0, 0, 512, 400, nullptr),
    gs{ _gs }
{
    this->padding.x = 0;
    this->padding.y = 48;

    this->current_texture = this->gs.tex_man.load(this->gs.package_man.get_unique("gfx/policies_screen.png"));
    this->width = this->current_texture->width;
    this->height = this->current_texture->height;
    this->text("Laws and goverment");
    this->is_scroll = false;
    this->new_policy = gs.curr_nation->current_policy;
    this->set_close_btn_function([this](Widget&) {
        this->kill();
    });

    new UI::Label(0, 0, "Goverment", this);
    auto* ideology_img = new UI::Image(6, 38, 32, 32, this);
    ideology_img->set_on_each_tick([this](UI::Widget& w) {
        if(this->gs.world->time % this->gs.world->ticks_per_month) return;
        /// @todo More dynamic names
        if(Ideology::is_valid(this->gs.curr_nation->ideology_id)) {
            auto& ideology = this->gs.world->ideologies[this->gs.curr_nation->ideology_id];
            w.text(ideology.name);
            w.current_texture = gs.tex_man.load(gs.package_man.get_unique(ideology.get_icon_path()));
        }
    });
    ideology_img->on_each_tick(*ideology_img);

    auto* ideology_pie_lab = new UI::Label(0, 82, "House", this);
    auto* ideology_pie = new UI::PieChart(0, 0, 128, 128, this);
    ideology_pie->below_of(*ideology_pie_lab);
    ideology_pie->set_on_each_tick([this](UI::Widget& w) {
        if(this->gs.world->time % this->gs.world->ticks_per_month) return;

        std::vector<UI::ChartData> ideology_data;
        for(const auto& ideology : this->gs.world->ideologies)
            ideology_data.push_back(UI::ChartData(1.f, ideology.name, ideology.color));
        
        for(const auto province_id : this->gs.curr_nation->owned_provinces)
            for(const auto& pop : this->gs.world->provinces[province_id].pops)
                for(size_t i = 0; i < pop.ideology_approval.size(); i++)
                    ideology_data[i].num += pop.ideology_approval[i] * pop.size;
        ((UI::PieChart&)w).set_data(ideology_data);
    });
    ideology_pie->on_each_tick(*ideology_pie);

    auto* militancy_lab = new UI::Label(0, 290, " ", this);
    militancy_lab->set_on_each_tick([this](UI::Widget& w) {
        if(this->gs.world->time % this->gs.world->ticks_per_month) return;

        auto num = 0.f;
        for(const auto province_id : this->gs.curr_nation->owned_provinces)
            for(const auto& pop : this->gs.world->provinces[province_id].pops)
                num += pop.militancy;
        num /= this->gs.curr_nation->owned_provinces.size();
        w.text("Militancy: " + std::to_string(num));
    });
    militancy_lab->on_each_tick(*militancy_lab);

    auto* reform_grp = new UI::Group(207, 38, this->width - 207, this->height - (this->padding.y + 38 + 48), this);
    reform_grp->is_scroll = true;

    // Social
    POLICY_CHECKBOX(slavery, "Slavery", "Allows to put the burden of work to slaves for free");

    POLICY_CHECKBOX(secular_education, "Secular education", "Educates people in a way that it's bound to religion");
    secular_education_chk->below_of(*slavery_chk);

    POLICY_CHECKBOX(private_property, "Private property", "Allow the right to own property");
    private_property_chk->below_of(*secular_education_chk);

    POLICY_CHECKBOX(public_education, "Public education", "(paid by the goverment)");
    public_education_chk->below_of(*private_property_chk);

    POLICY_CHECKBOX(public_healthcare, "Public healthcare", "(paid by the goverment)");
    public_healthcare_chk->below_of(*public_education_chk);

    POLICY_CHECKBOX(social_security, "Social security", "Help people in need with goverment support");
    social_security_chk->below_of(*public_healthcare_chk);

    auto* poor_tax_sld = new UI::Slider(0, 0, 128, 24, -1.f, 1.f, reform_grp);
    poor_tax_sld->below_of(*social_security_chk);
    poor_tax_sld->text(std::to_string(new_policy.poor_flat_tax));
    poor_tax_sld->set_value(new_policy.poor_flat_tax);
    poor_tax_sld->set_on_click([this](UI::Widget& w) {
        this->new_policy.poor_flat_tax = ((UI::Slider&)w).get_value();
        w.text(std::to_string(this->new_policy.poor_flat_tax * 100.f) + "%");
    });
    poor_tax_sld->set_tooltip("The taxing done to the low class (flat %)");

    auto* med_tax_sld = new UI::Slider(0, 0, 128, 24, -1.f, 1.f, reform_grp);
    med_tax_sld->below_of(*poor_tax_sld);
    med_tax_sld->text(std::to_string(new_policy.poor_flat_tax));
    med_tax_sld->set_value(new_policy.med_flat_tax);
    med_tax_sld->set_on_click([this](UI::Widget& w) {
        this->new_policy.med_flat_tax = ((UI::Slider&)w).get_value();
        w.text(std::to_string(this->new_policy.med_flat_tax * 100.f) + "%");
    });
    med_tax_sld->set_tooltip("The taxing done to the medium class (flat %)");

    auto* rich_tax_sld = new UI::Slider(0, 0, 128, 24, -1.f, 1.f, reform_grp);
    rich_tax_sld->below_of(*med_tax_sld);
    rich_tax_sld->text(std::to_string(new_policy.poor_flat_tax));
    rich_tax_sld->set_value(new_policy.rich_flat_tax);
    rich_tax_sld->set_on_click([this](UI::Widget& w) {
        this->new_policy.rich_flat_tax = ((UI::Slider&)w).get_value();
        w.text(std::to_string(this->new_policy.rich_flat_tax * 100.f) + "%");
    });
    rich_tax_sld->set_tooltip("The taxing done to the high class (flat %)");

    // Goverment structure
    POLICY_CHECKBOX(legislative_parliament, "Legislative parliament", "The legislative parliament allows to have a democratic vote on laws");
    legislative_parliament_chk->below_of(*rich_tax_sld);

    POLICY_CHECKBOX(executive_parliament, "Executive parliament", "The executive parliament will approve/disapprove acts of diplomacy and war");
    executive_parliament_chk->below_of(*legislative_parliament_chk);

    POLICY_CHECKBOX(constitutional, "Constitutional", "The constitution will limit the governing power of the goverment");
    constitutional_chk->below_of(*executive_parliament_chk);

    // Foreing policies
    POLICY_CHECKBOX(foreign_trade, "Foreign Trade", "Controls foreing trade with other countries");
    foreign_trade_chk->below_of(*constitutional_chk);

    auto* enact_btn = new UI::Button(207, 0, 128, 24, this);
    enact_btn->below_of(*reform_grp);
    enact_btn->text("Enact policy");
    enact_btn->set_on_click([this](UI::Widget&) {
        Eng3D::Networking::Packet packet{};
        Archive ar{};
        ::serialize<ActionType>(ar, ActionType::NATION_ENACT_POLICY);
        ::serialize(ar, this->new_policy); // PoliciesObj
        packet.data(ar.get_buffer(), ar.size());
        this->gs.client->send(packet);
        this->gs.ui_ctx.prompt("Policy", "New policy enacted!");
    });
}
