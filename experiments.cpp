/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 *                                                                           *
 *  This file is part of Library of Effective GO routines - EGO library      *
 *                                                                           *
 *  Copyright 2006, 2007 Lukasz Lew                                          *
 *                                                                           *
 *  EGO library is free software; you can redistribute it and/or modify      *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation; either version 2 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  EGO library is distributed in the hope that it will be useful,           *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with EGO library; if not, write to the Free Software               *
 *  Foundation, Inc., 51 Franklin St, Fifth Floor,                           *
 *  Boston, MA  02110-1301  USA                                              *
 *                                                                           *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


class AafStats {
public:
  Stat unconditional;
  FastMap<Move, Stat> given_move;

  void reset () {
    unconditional.reset ();
    move_for_each_all (m)
      given_move [m].reset ();
  }

  void update (Move* move_history, uint move_count, float score) {
    unconditional.update (score);
    rep (ii, move_count)
      given_move [move_history [ii]].update (score);
  }

  float norm_mean_given_move (const Move& m) {
    return given_move[m].mean () - unconditional.mean ();   // ineffective in loop
  }
};


class AllAsFirst : public GtpEngine {
public:
  Board*      board;
  AafStats    aaf_stats;
  uint        playout_no;
  float       aaf_fraction;
  float       influence_scale;
  
public:
  AllAsFirst (Gtp& gtp, Board& board_) : board (&board_) { 
    playout_no = 50000;
    aaf_fraction = 0.5;
    influence_scale = 6.0;

    gtp.add_gogui_command (this, "dboard", "AAF.move_value",          "black");
    gtp.add_gogui_command (this, "dboard", "AAF.move_value",          "white");
    gtp.add_gogui_command (this, "none",   "AAF.set_influence_scale", "%s");
    gtp.add_gogui_command (this, "none",   "AAF.set_playout_number",  "%s");
    gtp.add_gogui_command (this, "none",   "AAF.set_aaf_fraction",    "%s");
  }
    
  void do_playout (const Board* base_board) {
    Board mc_board [1];
    mc_board->load (base_board);
    Playout<SimplePolicy> (mc_board).run ();

    float score = mc_board->score ();
    uint aaf_move_count = uint (float(mc_board->move_no)*aaf_fraction);

    aaf_stats.update (mc_board->move_history, aaf_move_count, score);
  }

  virtual GtpStatus exec_command (string command, istream& params, ostream& response) {
    if (command == "AAF.move_value") {
      Player player;
      if (!(params >> player)) return gtp_syntax_error;
      aaf_stats.reset ();
      rep (ii, playout_no) 
        do_playout (board);
      FastMap<Vertex, float> means;
      vertex_for_each_all (v) {
        means [v] = aaf_stats.norm_mean_given_move (Move(player, v)) / influence_scale;;
        if (board->color_at [v] != Color::empty ()) 
          means [v] = 0.0;
      }
      response << to_string_2d (means);
      return gtp_success;
    }

    if (command == "AAF.set_influence_scale") {
      if (!(params >> influence_scale)) {
        response << "influence_scale = " << influence_scale;
        return gtp_failure;
      }
      return gtp_success;
    }

    if (command == "AAF.set_playout_number") {
      if (!(params >> playout_no)) {
        response << "playout_number = " << playout_no;
        return gtp_failure;
      }
      return gtp_success;
    }

    if (command == "AAF.set_aaf_fraction") {
      if (!(params >> aaf_fraction)) {
        response << "aaf_fraction = " << aaf_fraction;
        return gtp_failure;
      }
      return gtp_success;
    }

    fatal_error ("wrong command in AllAsFirst::exec_command");
    return gtp_panic; // formality 
  }

};
