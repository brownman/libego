/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 *                                                                           *
 *  This file is part of Library of Effective GO routines - EGO library      *
 *                                                                           *
 *  Copyright 2006 and onwards, Lukasz Lew                                   *
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




// class gtp_sgf

class GtpSgf : public GtpCommand {
public:
  SgfTree&  sgf_tree;
  Gtp&      gtp;
  Board&    base_board;


  GtpSgf (Gtp& _gtp, SgfTree& _sgf_tree, Board& _base_board) : 
    sgf_tree (_sgf_tree), gtp (_gtp), base_board (_base_board) {
    gtp.add_gtp_command (this, "sgf.load");
    gtp.add_gtp_command (this, "sgf.save");
    gtp.add_gtp_command (this, "sgf.gtp.exec");
  }

  virtual GtpResult exec_command (string command, istream& params) {
    // ---------------------------------------------------------------------
    if (command == "sgf.load") {
      string file_name;

      if (!(params >> file_name)) return GtpResult::syntax_error ();

      ifstream sgf_stream (file_name.data ());
      if (!sgf_stream) {
        return GtpResult::failure ("file not found: " + file_name);
      }

      if (sgf_tree.parse_sgf (sgf_stream) == false) {
        return GtpResult::failure ("invalid SGF file");
      }

      return GtpResult::success ();
    }

    // ---------------------------------------------------------------------
    if (command == "sgf.save") {
      string file_name;

      if (!(params >> file_name)) return GtpResult::syntax_error ();

      ofstream sgf_stream (file_name.data ());
      if (!sgf_stream) {
        return GtpResult::failure ("file cound not be created: " + file_name);
      }
      
      sgf_stream << sgf_tree.to_sgf_string () << endl;
      sgf_stream.close ();

      return GtpResult::success ();
    }

    // ---------------------------------------------------------------------
    if (command == "sgf.gtp.exec") {
      if (!sgf_tree.is_loaded ()) {
        return GtpResult::failure ("SGF file not loaded");
      }

      if (sgf_tree.properties ().get_board_size () != board_size) {
        return GtpResult::failure ("invalid board size");
      }

      Board save_board;
      save_board.load (&base_board);
      
      base_board.clear ();
      base_board.set_komi (sgf_tree.properties ().get_komi ());

      ostringstream response;
      exec_embedded_gtp_rec (sgf_tree.game_node (), response);

      base_board.load (&save_board);

      return GtpResult::success (response.str ());
    }

    // ---------------------------------------------------------------------
    fatal_error ("this should not happen!: error number = 0x994827");
    assert (false);
  }

  
  void exec_embedded_gtp_rec (SgfNode* current_node, ostream& response) {
    // update position
    list <Vertex> vertex_list;
    vertex_list = current_node->properties.get_vertices_to_play (Color::empty ());
    if (vertex_list.empty () == false) {
      response << "sgf.gtp.exec: AE property in SGF not implemented" << endl;
      return;
    }

    player_for_each (pl) {
      vertex_list = current_node->properties.get_vertices_to_play (pl);
      for_each (vi, vertex_list) {
        if (base_board.try_play (pl, *vi) == false) {
          response << "sgf.gtp.exec: bad move in SGF: " << pl << " " << *vi << endl;
          return;
        }
      }
    }

    // save board
    Board node_board;
    node_board.load (&base_board);

    // process gtp
    istringstream embedded_gtp (current_node->properties.get_embedded_gtp ());
    ostringstream embedded_response;
    gtp.run_loop (embedded_gtp, embedded_response, true); // TODO make gtp filter out double newlines
    current_node->properties.set_comment (embedded_response.str ());

    // recursove call
    for_each (child, current_node->children) {
      base_board.load (&node_board);
      exec_embedded_gtp_rec (&(*child), response); // LOL itearator is imitationg a pointer but we need a true pointer
    }
    
  }
  
};