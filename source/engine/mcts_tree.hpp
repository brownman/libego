#ifndef MCTS_TREE_
#define MCTS_TREE_

#include <list>

const bool mcts_tree_ac = true;

// -----------------------------------------------------------------------------
// TODO move to separate class and static object

// -----------------------------------------------------------------------------

class MctsNode {
public:
  typedef std::list<MctsNode> ChildrenList; // TODO vector, allocator?

  // Initialization.

  explicit MctsNode (Player player_, Vertex v_);

  void Reset ();

  // Printing.

  string ToString() const;

  string RecToString (float min_visit, uint max_children) const; 

  // Children operations.
  
  ChildrenList& Children ();

  void AddChild (const MctsNode& node);

  void RemoveChild (MctsNode* child_ptr);

  bool ReadyToExpand () const;

  void EnsureAllLegalChildren (Player pl, const Board& board);

  void RemoveIllegalChildren (Player pl, const FullBoard& full_board);

  // Child finding.

  MctsNode* FindChild (Move m);

  const MctsNode& MostExploredChild (Player pl) const;

  MctsNode& BestRaveChild (Player pl);

  // Other.
  
  float SubjectiveMean() const;

  float SubjectiveRaveValue (Player pl, float log_val) const;

public:

  Move GetMove () const;

  Player player;
  Vertex v;
  NatMap <Player, bool> has_all_legal_children;

  Stat stat;
  Stat rave_stat;

private:
  void RecPrint (ostream& out, uint depth, float min_visit, uint max_children) const;

  ChildrenList children;

  static const bool kCheckAsserts = false;
};

// -----------------------------------------------------------------------------

MctsNode::MctsNode (Player player_, Vertex v_)
: player(player_), v(v_), has_all_legal_children (false) {
  Reset ();
}

MctsNode::ChildrenList& MctsNode::Children () {
  return children;
}

Move MctsNode::GetMove () const {
  return Move(player, v);
}

void MctsNode::AddChild (const MctsNode& node) {
  children.push_front (node);
}

// TODO better implementation of child removation.
void MctsNode::RemoveChild (MctsNode* child_ptr) {
  ChildrenList::iterator child = children.begin();
  while (true) {
    ASSERT (child != children.end());
    if (&*child == child_ptr) {
      children.erase(child);
      return;
    }
    child++;
  }
}

bool MctsNode::ReadyToExpand () const {
  return stat.update_count() > 
    Param::prior_update_count + Param::mature_update_count;
}

MctsNode* MctsNode::FindChild (Move m) {
  // TODO make invariant about haveChildren and has_all_legal_children
  Player pl = m.GetPlayer();
  Vertex v  = m.GetVertex();
  ASSERT (has_all_legal_children [pl]);
  BOOST_FOREACH (MctsNode& child, children) {
    if (child.player == pl && child.v == v) {
      return &child;
    }
  }
  FAIL ("should not happen");
}

string MctsNode::ToString() const {
  stringstream s;
  s << player.ToGtpString() << " " 
    << v.ToGtpString() << " " 
    << stat.to_string() << " "
    << rave_stat.to_string() << " -> "
    << Stat::Mix (stat,      Param::tree_stat_bias,
                  rave_stat, Param::tree_rave_bias)
    // << " - ("  << stat.precision (Param::mcts_bias) << " : "
    // << stat.precision (Param::rave_bias) << ")"
    // << Stat::SlowMix (stat,
    //                   Param::mcts_bias,
    //                   rave_stat,
    //                   Param::rave_bias)
    ;

  return s.str();
}

namespace {
  bool SubjectiveCmp (const MctsNode* a, const MctsNode* b) {
    return a->stat.update_count() > b->stat.update_count();
    // return SubjectiveMean () > b->SubjectiveMean ();
  }
}

void MctsNode::RecPrint (ostream& out, uint depth, float min_visit, uint max_children) const {
  rep (d, depth) out << "  ";
  out << ToString () << endl;

  vector <const MctsNode*> child_tab;
  BOOST_FOREACH (const MctsNode& child, children) {
    child_tab.push_back(&child);
  }

  sort (child_tab.begin(), child_tab.end(), SubjectiveCmp);
  if (child_tab.size () > max_children) child_tab.resize(max_children);

  BOOST_FOREACH (const MctsNode* child, child_tab) {
    if (child->stat.update_count() >= min_visit) {
      child->RecPrint (out, depth + 1, min_visit, max(1u, max_children - 1));
    }
  }
}

string MctsNode::RecToString (float min_visit, uint max_children) const { 
  ostringstream out;
  RecPrint (out, 0, min_visit, max_children); 
  return out.str ();
}

const MctsNode& MctsNode::MostExploredChild (Player pl) const {
  const MctsNode* best = NULL;
  float best_update_count = -1;

  ASSERT (has_all_legal_children [pl]);

  BOOST_FOREACH (const MctsNode& child, children) {
    if (child.player == pl && child.stat.update_count() > best_update_count) {
      best_update_count = child.stat.update_count();
      best = &child;
    }
  }

  ASSERT (best != NULL);
  return *best;
}


MctsNode& MctsNode::BestRaveChild (Player pl) {
  MctsNode* best_child = NULL;
  float best_urgency = -100000000000000.0; // TODO infinity
  const float log_val = log (stat.update_count());

  ASSERT (has_all_legal_children [pl]);

  BOOST_FOREACH (MctsNode& child, Children()) {
    if (child.player != pl) continue;
    float child_urgency = child.SubjectiveRaveValue (pl, log_val);
    if (child_urgency > best_urgency) {
      best_urgency = child_urgency;
      best_child   = &child;
    }
  }

  ASSERT (best_child != NULL); // at least pass
  return *best_child;
}


void MctsNode::EnsureAllLegalChildren (Player pl, const Board& board) {
  if (has_all_legal_children [pl]) return;
  empty_v_for_each_and_pass (&board, v, {
      // superko nodes have to be removed from the tree later
      if (board.IsLegal (pl, v))
        AddChild (MctsNode(pl, v));
    });
  has_all_legal_children [pl] = true;
}

void MctsNode::RemoveIllegalChildren (Player pl, const FullBoard& full_board) {
  ASSERT (has_all_legal_children [pl]);

  ChildrenList::iterator child = children.begin();
  while (child != children.end()) {
    if (child->player == pl && !full_board.IsLegal (Move (pl, child->v))) {
      children.erase (child++);
    } else {
      ++child;
    }
  }
}

void MctsNode::Reset () {
  has_all_legal_children.SetAll (false);
  children.clear ();
  stat.reset      (Param::prior_update_count,
                   player.SubjectiveScore (Param::prior_mean));
  rave_stat.reset (Param::prior_update_count,
                   player.SubjectiveScore (Param::prior_mean));
}

float MctsNode::SubjectiveMean () const {
  return player.SubjectiveScore (stat.mean ());
}

float MctsNode::SubjectiveRaveValue (Player pl, float log_val) const {
  float value;

  if (Param::tree_rave_use) {
    value = Stat::Mix (stat,      Param::tree_stat_bias,
                       rave_stat, Param::tree_rave_bias);
  } else {
    value = stat.mean ();
  }

  return
    pl.SubjectiveScore (value) +
    Param::tree_explore_coeff * sqrt (log_val / stat.update_count());
}

#endif
