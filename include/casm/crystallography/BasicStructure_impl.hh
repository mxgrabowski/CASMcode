#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#include "casm/crystallography/UnitCellCoord.hh"
#include "casm/crystallography/PrimGrid.hh"
#include "casm/symmetry/SymPermutation.hh"
#include "casm/symmetry/SymBasisPermute.hh"
#include "casm/symmetry/SymGroupRep.hh"

namespace CASM {
  template<typename CoordType>
  BasicStructure<CoordType>::BasicStructure(const fs::path &filepath) : m_lattice() {
    if(!fs::exists(filepath)) {
      std::cout << "Error in BasicStructure<CoordType>::BasicStructure<CoordType>(const fs::path &filepath)." << std::endl;
      std::cout << "  File does not exist at: " << filepath << std::endl;
      exit(1);
    }
    fs::ifstream infile(filepath);

    read(infile);
  }

  //***********************************************************

  template<typename CoordType>
  BasicStructure<CoordType>::BasicStructure(const BasicStructure &RHS) :
    m_lattice(RHS.lattice()), title(RHS.title), basis(RHS.basis) {
    for(Index i = 0; i < basis.size(); i++) {
      basis[i].set_lattice(lattice());
    }
  }

  //***********************************************************

  template<typename CoordType>
  BasicStructure<CoordType> &BasicStructure<CoordType>::operator=(const BasicStructure<CoordType> &RHS) {
    m_lattice = RHS.lattice();
    title = RHS.title;
    basis = RHS.basis;
    for(Index i = 0; i < basis.size(); i++) {
      basis[i].set_lattice(lattice());
    }

    return *this;
  }


  //***********************************************************

  template<typename CoordType>
  void BasicStructure<CoordType>::copy_attributes_from(const BasicStructure<CoordType> &RHS) {

  }

  //***********************************************************
  /*
  template<typename CoordType>
  BasicStructure<CoordType> &BasicStructure<CoordType>::apply_sym(const SymOp &op) {
    for(Index i = 0; i < basis.size(); i++) {
      basis[i].apply_sym(op);
    }
    return *this;
  }
  */
  //***********************************************************

  template<typename CoordType>
  void BasicStructure<CoordType>::reset() {


    for(Index nb = 0; nb < basis.size(); nb++) {
      basis[nb].set_basis_ind(nb);
    }
    within();
    //set_site_internals();

  }

  //***********************************************************

  template<typename CoordType>
  void BasicStructure<CoordType>::update() {
    set_site_internals();
  }

  //***********************************************************

  template<typename CoordType>
  void BasicStructure<CoordType>::within() {
    for(Index i = 0; i < basis.size(); i++) {
      basis[i].within();
    }
    return;
  }

  //***********************************************************

  template<typename CoordType>
  void BasicStructure<CoordType>::generate_factor_group_slow(SymGroup &factor_group, double map_tol) const {
    //std::cout << "SLOW GENERATION OF FACTOR GROUP " << &factor_group << "\n";
    //std::cout << "begin generate_factor_group_slow() " << this << std::endl;

    Array<CoordType> tsite;
    Index pg, b0, b1, b2;
    Coordinate t_tau(lattice());
    Index num_suc_maps;

    SymGroup point_group;
    //reset();
    lattice().generate_point_group(point_group, map_tol);

    if(factor_group.size() != 0) {
      std::cerr << "WARNING in BasicStructure<CoordType>::generate_factor_group_slow" << std::endl;
      std::cerr << "The factor group passed isn't empty and it's about to be rewritten!" << std::endl;
      factor_group.clear();
    }

    //Loop over all point group ops of the lattice
    for(pg = 0; pg < point_group.size(); pg++) {
      tsite.clear();
      //First, generate the symmetrically transformed basis sites
      //Loop over all sites in basis
      for(b0 = 0; b0 < basis.size(); b0++) {
        tsite.push_back(point_group[pg]*basis[b0]);
      }

      //Using the symmetrically transformed basis, find all possible translations
      //that MIGHT map the symmetrically transformed basis onto the original basis
      for(b0 = 0; b0 < tsite.size(); b0++) {

        if(!basis[0].compare_type(tsite[b0]))
          continue;

        t_tau = basis[0] - tsite[b0];

        t_tau.within();
        num_suc_maps = 0; //Keeps track of number of old->new basis site mappings that are found

        double tdist = 0.0;
        double max_error = 0.0;
        for(b1 = 0; b1 < basis.size(); b1++) { //Loop over original basis sites
          for(b2 = 0; b2 < tsite.size(); b2++) { //Loop over symmetrically transformed basis sites

            //see if translation successfully maps the two sites
            if(basis[b1].compare(tsite[b2], t_tau, map_tol)) {
              tdist = basis[b1].min_dist(tsite[b2], t_tau);
              if(tdist > max_error) {
                max_error = tdist;
              }
              num_suc_maps++;
              break;
            }
          }

          //break out of outer loop if inner loop finds no successful map
          if(b2 == tsite.size()) {
            break;
          }
        }

        //If all atoms in the basis are mapped successfully, try to add the corresponding
        //symmetry operation to the factor_group
        if(num_suc_maps == basis.size()) {
          SymOp tSym(SymOp(t_tau)*point_group[pg]);
          tSym.set_map_error(max_error);

          if(!factor_group.contains(tSym)) {
            factor_group.push_back(tSym);
          }
        }
      }
    } //End loop over point_group operations
    factor_group.enforce_group(map_tol);
    factor_group.sort_by_class();
    factor_group.get_max_error();

    //std::cout << "finish generate_factor_group_slow() " << this << std::endl;
    return;
  }

  //************************************************************

  template<typename CoordType>
  void BasicStructure<CoordType>::generate_factor_group(SymGroup &factor_group, double map_tol) const {
    //std::cout << "begin generate_factor_group() " << this << std::endl;
    BasicStructure<CoordType> tprim;
    factor_group.clear();

    // CASE 1: Structure is primitive
    if(is_primitive(tprim, map_tol)) {
      generate_factor_group_slow(factor_group, map_tol);
      return;
    }


    // CASE 2: Structure is not primitive

    PrimGrid prim_grid(tprim.lattice(), lattice());
    //std::cout << "FAST GENERATION OF FACTOR GROUP " << &factor_group << " FOR STRUCTURE OF VOLUME " << prim_grid.size() << "\n";

    SymGroup prim_fg;
    tprim.generate_factor_group_slow(prim_fg, map_tol);

    SymGroup point_group;
    lattice().generate_point_group(point_group, map_tol);
    point_group.enforce_group(map_tol);

    for(Index i = 0; i < prim_fg.size(); i++) {
      if(point_group.find_no_trans(prim_fg[i]) == point_group.size()) {
        continue;
      }
      else {
        for(Index j = 0; j < prim_grid.size(); j++) {
          factor_group.push_back(SymOp(prim_grid.coord(j, SCEL))*prim_fg[i]);
          // set lattice, in case SymOp::operator* ever changes
          factor_group.back().set_lattice(lattice(), CART);
        }
      }
    }

    return;
  }

  //************************************************************

  template<typename CoordType>
  void BasicStructure<CoordType>::fg_converge(double small_tol, double large_tol, double increment) {
    SymGroup factor_group;
    fg_converge(factor_group, small_tol, large_tol, increment);
  }

  //************************************************************

  template<typename CoordType>
  void BasicStructure<CoordType>::fg_converge(SymGroup &factor_group, double small_tol, double large_tol, double increment) {

    Array<double> tols;
    Array<bool> is_group;
    Array<int> num_ops, num_enforced_ops;
    Array<std::string> name;

    for(double i = small_tol; i < large_tol; i += increment) {
      tols.push_back(i);
      factor_group.clear();
      generate_factor_group(factor_group, i);
      factor_group.get_multi_table();
      num_ops.push_back(factor_group.size());
      is_group.push_back(factor_group.is_group(i));
      factor_group.enforce_group(i);
      num_enforced_ops.push_back(factor_group.size());
      factor_group.get_character_table();
      name.push_back(factor_group.get_name());
    }

    for(Index i = 0; i < tols.size(); i++) {
      std::cout << tols[i] << "\t" << num_ops[i] << "\t" << is_group[i] << "\t" << num_enforced_ops[i] << "\t name: " << name[i] << "\n";
    }

    return;
  }

  //***********************************************************

  //This function gets the permutation representation of the
  // factor group operations of the structure. It first applies
  // the factor group operation to the structure, and then tries
  // to map the new position of the basis atom to the various positions
  // before symmetry was applied. It only checks the positions after
  // it brings the basis within the crystal.

  // ROUTINE STILL NEEDS TO BE TESTED!

  // ARN 0531

  template<typename CoordType>
  Index BasicStructure<CoordType>::generate_permutation_representation(const MasterSymGroup &factor_group, bool verbose) const {
    if(factor_group.size() <= 0) {
      std::cerr << "ERROR in BasicStructure::generate_permutation_representation" << std::endl;
      std::cerr << "You have NOT generated the factor group, or something is very wrong with your structure. I'm quitting!" << std::endl;;
      exit(1);
    }
    SymGroupRep permute_group(factor_group);
    Index rep_id;
    CoordType tsite(basis[0]);
    CASM::BasicStructure<CoordType> sym_tstruc = (*this);
    Array<Index> tArr;

    std::string clr(100, ' ');

    for(Index symOp_num = 0; symOp_num < factor_group.size(); symOp_num++) {
      if(verbose) {
        if(symOp_num % 100 == 0)
          std::cout << '\r' << clr.c_str() << '\r' << "Find permute rep for symOp " << symOp_num << "/" << factor_group.size() << std::flush;
      }
      int flag;
      tArr.clear();
      // Applies the symmetry operation
      //sym_tstruc = factor_group[symOp_num] * (*this);  <-- slow due to memory copying
      for(Index i = 0; i < basis.size(); i++) {
        tsite = basis[i];
        sym_tstruc.basis[i] = tsite.apply_sym(factor_group[symOp_num]);
      }
      for(Index i = 0; i < sym_tstruc.basis.size(); i++) {
        // Moves the atom within the unit cell
        sym_tstruc.basis[i].within();
        flag = 0;
        // tries to locate the transformed basis in the untransformed structure
        for(Index j = 0; j < basis.size(); j++) {
          if(sym_tstruc.basis[i].compare(basis[j])) {
            tArr.push_back(j);
            flag = 1;
            break;
          }
        }
        if(!flag) {
          //Quits if it wasnt able to perform the mapping
          std::cerr << "\nWARNING:  In BasicStructure::generate_permutation_representation ---"
                    << "            Something is wrong with your factor group operations. I\'m quitting!\n";
          exit(1);
        }
      }
      // Creates a new SymGroupRep
      permute_group.push_back(SymPermutation(tArr));
    }
    // Adds the representation into the master sym group of this structure and returns the rep id
    rep_id = factor_group.add_representation(permute_group);

    if(verbose) std::cout << '\r' << clr.c_str() << '\r' << std::flush;
    return rep_id;
  }

  //***********************************************************

  //This function gets the permutation representation of the
  // factor group operations of the structure. It first applies
  // the factor group operation to the structure, and then tries
  // to map the new position of the basis atom to the various positions
  // before symmetry was applied. It only checks the positions after
  // it brings the basis within the crystal.

  template<typename CoordType>
  Index BasicStructure<CoordType>::generate_basis_permutation_representation(const MasterSymGroup &factor_group, bool verbose) const {

    if(factor_group.size() <= 0 || !basis.size()) {
      std::cerr << "ERROR in BasicStructure::generate_basis_permutation_representation" << std::endl;
      std::cerr << "You have NOT generated the factor group, or something is very wrong with your structure. I'm quitting!" << std::endl;;
      exit(1);
    }

    SymGroupRep basis_permute_group(factor_group);
    Index rep_id;

    CoordType tsite(basis[0]);
    Array<UnitCellCoord> new_ucc(basis.size());

    std::string clr(100, ' ');

    for(Index ng = 0; ng < factor_group.size(); ng++) {
      if(verbose) {
        if(ng % 100 == 0)
          std::cout << '\r' << clr.c_str() << '\r' << "Find permute rep for symOp " << ng << "/" << factor_group.size() << std::flush;
      }

      // Applies the symmetry operation
      //sym_tstruc = factor_group[ng] * (*this);  <-- slow due to memory copying
      for(Index nb = 0; nb < basis.size(); nb++) {
        tsite = basis[nb];
        tsite.apply_sym(factor_group[ng]);
        new_ucc[nb] = get_unit_cell_coord(tsite);
      }
      // Creates a new SymGroupRep
      basis_permute_group.push_back(SymBasisPermute(new_ucc));
    }
    // Adds the representation into the master sym group of this structure and returns the rep id
    rep_id = factor_group.add_representation(basis_permute_group);

    //std::cerr << "Added basis permutation rep id " << rep_id << '\n';

    if(verbose) std::cout << '\r' << clr.c_str() << '\r' << std::flush;
    return rep_id;
  }


  //***********************************************************
  /**
   * It is NOT wise to use this function unless you have already
   * initialized a superstructure with lattice vectors.
   *
   * It is more wise to use the two methods that call this method:
   * Either the overloaded * operator which does:
   *  SCEL_Lattice * Prim_Structrue = New_Superstructure
   *       --- or ---
   *  New_Superstructure=Prim_BasicStructure<CoordType>.create_superstruc(SCEL_Lattice);
   *
   *  Both of these will return NEW superstructures.
   */
  //***********************************************************

  template<typename CoordType>
  void BasicStructure<CoordType>::fill_supercell(const BasicStructure<CoordType> &prim, double map_tol) {
    Index i, j;

    //Updating lattice_trans matrices that connect the supercell to primitive -- Changed 11/08/12
    m_lattice.calc_conversions();

    copy_attributes_from(prim);

    PrimGrid prim_grid(prim.lattice(), lattice());

    basis.clear();

    //loop over basis sites of prim
    for(j = 0; j < prim.basis.size(); j++) {

      //loop over prim_grid points
      for(i = 0; i < prim_grid.size(); i++) {

        //push back translated basis site of prim onto superstructure basis
        basis.push_back(prim.basis[j] + prim_grid.coord(i, PRIM));

        //reset lattice for most recent superstructure CoordType
        //set_lattice() converts fractional coordinates to be compatible with new lattice
        basis.back().set_lattice(lattice(), CART);

        basis.back().within();
        for(Index k = 0; k + 1 < basis.size(); k++) {
          if(basis[k].compare(basis.back(), map_tol)) {
            basis.pop_back();
            break;
          }
        }
      }
    }

    set_site_internals();

    return;
  }

  //***********************************************************
  /**
   * Operates on the primitive structure and takes as an argument
   * the supercell lattice.  It then returns a new superstructure.
   *
   * This is similar to the Lattice*Primitive routine which returns a
   * new superstructure.  Unlike the fill_supercell routine which takes
   * the primitive structure, this WILL fill the sites.
   */
  //***********************************************************

  template<typename CoordType>
  BasicStructure<CoordType> BasicStructure<CoordType>::create_superstruc(const Lattice &scel_lat, double map_tol) const {
    BasicStructure<CoordType> tsuper(scel_lat);
    tsuper.fill_supercell(*this);
    return tsuper;
  }


  //***********************************************************
  /**
   * Determines if structure is primitive description of the crystal
   */
  //***********************************************************

  template<typename CoordType>
  bool BasicStructure<CoordType>::is_primitive(double prim_tol) const {
    Coordinate tshift(lattice());//, bshift(lattice);
    Index b1, b2, b3, num_suc_maps;

    for(b1 = 1; b1 < basis.size(); b1++) {
      if(!basis[0].compare_type(basis[b1])) {
        continue;
      }

      tshift = basis[0] - basis[b1];
      num_suc_maps = 0;
      for(b2 = 0; b2 < basis.size(); b2++) {
        for(b3 = 0; b3 < basis.size(); b3++) {
          //if(basis[b3].compare_type(basis[b2], bshift) && tshift.min_dist(bshift) < prim_tol) {
          if(basis[b3].compare(basis[b2], tshift, prim_tol)) {
            num_suc_maps++;
            break;
          }
        }
        if(b3 == basis.size()) {
          break;
        }
      }

      if(num_suc_maps == basis.size()) {
        return false;
      }

    }

    return true;
  }


  //***********************************************************
  /**
   * Determines if structure is primitive description of the crystal
   * If not, finds primitive cell and copies to new_prim
   */
  //***********************************************************

  template<typename CoordType>
  bool BasicStructure<CoordType>::is_primitive(BasicStructure<CoordType> &new_prim, double prim_tol) const {
    Coordinate tshift(lattice());//, bshift(lattice);
    Vector3<double> prim_vec0(lattice()[0]), prim_vec1(lattice()[1]), prim_vec2(lattice()[2]);
    Array<Vector3< double > > shift;
    Index b1, b2, b3, sh, sh1, sh2;
    Index num_suc_maps;
    double tvol, min_vol;
    bool prim_flag = true;
    double prim_vol_tol = std::abs(0.5 * lattice().vol() / double(basis.size())); //sets a hard lower bound for the minimum value of the volume of the primitive cell

    for(b1 = 1; b1 < basis.size(); b1++) {
      tshift = basis[0] - basis[b1];
      num_suc_maps = 0;
      for(b2 = 0; b2 < basis.size(); b2++) {
        for(b3 = 0; b3 < basis.size(); b3++) {
          if(basis[b3].compare(basis[b2], tshift, prim_tol)) {
            num_suc_maps++;
            break;
          }
        }
        if(b3 == basis.size()) {
          break;
        }
      }
      if(num_suc_maps == basis.size()) {
        prim_flag = false;
        shift.push_back(tshift(CART));
      }
    }

    if(prim_flag) {
      new_prim = (*this);
      return true;
    }

    shift.push_back(lattice()[0]);
    shift.push_back(lattice()[1]);
    shift.push_back(lattice()[2]);

    //We want to minimize the volume of the primitivized cell, but to make it not a weird shape
    //that leads to noise we also minimize the dot products like get_reduced cell would
    min_vol = std::abs(lattice().vol());
    for(sh = 0; sh < shift.size(); sh++) {
      for(sh1 = sh + 1; sh1 < shift.size(); sh1++) {
        for(sh2 = sh1 + 1; sh2 < shift.size(); sh2++) {
          tvol = std::abs(triple_prod(shift[sh], shift[sh1], shift[sh2]));
          if(tvol < min_vol && tvol > prim_vol_tol) {
            min_vol = tvol;
            prim_vec0 = shift[sh];
            prim_vec1 = shift[sh1];
            prim_vec2 = shift[sh2];
          }
        }
      }

    }


    Lattice new_lat(prim_vec0, prim_vec1, prim_vec2);
    Lattice reduced_new_lat = new_lat.get_reduced_cell();

    //The lattice so far is OK, but it's noisy enough to matter for large
    //superstructures. We eliminate the noise by reconstructing it now via
    //rounded to integer transformation matrix.

    Matrix3<double> transmat, invtransmat, reduced_new_lat_mat;
    SymGroup pgroup;
    reduced_new_lat.generate_point_group(pgroup, prim_tol);

    //Do not check to see if it returned true, it very well may not!
    lattice().is_supercell_of(reduced_new_lat, pgroup, transmat);

    //Round transformation elements to integers
    for(int i = 0; i < 3; i++) {
      for(int j = 0; j < 3; j++) {
        transmat(i, j) = floor(transmat(i, j) + 0.5);
      }
    }

    invtransmat = transmat.inverse();
    reduced_new_lat_mat = lattice().lat_column_mat();
    //When constructing this, why are we using *this as the primitive cell? Seems like I should only specify the vectors
    Lattice reconstructed_reduced_new_lat(reduced_new_lat_mat * invtransmat);
    //Lattice reconstructed_reduced_new_lat(reduced_new_lat_mat*invtransmat,lattice);

    new_prim.set_lattice(reconstructed_reduced_new_lat, CART);
    CoordType tsite(new_prim.lattice());
    for(Index nb = 0; nb < basis.size(); nb++) {
      tsite = basis[nb];
      tsite.set_lattice(new_prim.lattice(), CART);
      if(new_prim.find(tsite, prim_tol) == new_prim.basis.size()) {
        tsite.within();
        new_prim.basis.push_back(tsite);
      }
    }

    //std::cout<<"%%%%%%%%%%%%%%%%"<<std::endl;
    //std::cout<<"new_lat"<<std::endl;
    //new_lat.print(std::cout);
    //std::cout<<"reduced_new_lat"<<std::endl;
    //reduced_new_lat.print(std::cout);
    //std::cout<<"reconstructed_reduced_new_lat"<<std::endl;
    //reconstructed_reduced_new_lat.print(std::cout);
    //std::cout<<"transmat (rounded)"<<std::endl;
    //std::cout<<transmat<<std::endl;
    //std::cout<<"%%%%%%%%%%%%%%%%"<<std::endl;
    return false;
  }

  //***********************************************************

  template<typename CoordType>
  void BasicStructure<CoordType>::set_site_internals() {
    //std::cout << "begin set_site_internals() " << this << std::endl;
    Index nb;


    for(nb = 0; nb < basis.size(); nb++) {
      basis[nb].set_basis_ind(nb);
    }

  }

  //*********************************************************

  template<typename CoordType>
  void BasicStructure<CoordType>::map_superstruc_to_prim(BasicStructure<CoordType> &prim, const SymGroup &point_group) {

    int prim_to_scel = -1;
    CoordType shifted_site(prim.lattice());

    //Check that (*this) is actually a supercell of the prim
    if(!lattice().is_supercell_of(prim.lattice(), point_group)) {
      std::cout << "*******************************************\n"
                << "ERROR in BasicStructure<CoordType>::map_superstruc_to_prim:\n"
                << "The structure \n";
      print(std::cout);
      std::cout << "is not a supercell of the given prim!\n";
      prim.print(std::cout);
      std::cout << "*******************************************\n";
      exit(1);
    }


    // This is needed to update conversions from primitive to supercell -- probably no longer necessary
    m_lattice.calc_conversions();
    m_lattice.calc_properties(); // This updates prim_vol

    //Get prim grid of supercell to get the lattice translations
    //necessary to stamp the prim in the superstructure
    PrimGrid prim_grid(prim.lattice(), lattice());

    // Translate each of the prim atoms by prim_grid translation
    // vectors, and map that translated atom in the supercell.
    for(Index pg = 0; pg < prim_grid.size(); pg++) {
      for(Index pb = 0; pb < prim.basis.size(); pb++) {
        shifted_site = prim.basis[pb];
        //shifted_site lattice is PRIM, so get prim_grid.coord in PRIM mode
        shifted_site += prim_grid.coord(pg, PRIM);
        shifted_site.set_lattice(lattice(), CART);
        shifted_site.within();

        // invalidate asym_ind and basis_ind because when we use
        // BasicStructure<CoordType>::find, we don't want a comparison using the
        // basis_ind and asym_ind; we want a comparison using the
        // cartesian and Specie type.

        shifted_site.set_basis_ind(-1);
        prim_to_scel = find(shifted_site);

        if(prim_to_scel == basis.size()) {
          std::cout << "*******************************************\n"
                    << "ERROR in BasicStructure<CoordType>::map_superstruc_to_prim:\n"
                    << "Cannot associate site \n"
                    << shifted_site << "\n"
                    << "with a site in the supercell basis. \n"
                    << "*******************************************\n";
          std::cout << "The basis_ind and asym_ind are "
                    << shifted_site.basis_ind() << "\t "
                    << shifted_site.asym_ind() << "\n";
          exit(2);
        }

        // Set ind_to_prim of the basis site
        basis[prim_to_scel].ind_to_prim = pb;
      }
    }
  }

  //***********************************************************

  template<typename CoordType> template<typename CoordType2>
  Index BasicStructure<CoordType>::find(const CoordType2 &test_site, double tol) const {
    for(Index i = 0; i < basis.size(); i++) {
      if(basis[i].compare(test_site, tol)) {
        return i;
      }
    }
    return basis.size();
  }

  //***********************************************************

  template<typename CoordType> template<typename CoordType2>
  Index BasicStructure<CoordType>::find(const CoordType2 &test_site, const Coordinate &shift, double tol) const {
    for(Index i = 0; i < basis.size(); i++) {
      if(basis[i].compare(test_site, shift, tol)) {
        return i;
      }
    }
    return basis.size();
  }

  //John G 070713
  //***********************************************************
  /**
   * Using the lattice of (*this), this function will return
   * a UnitCellCoord that corresponds to a site passed to it
   * within a given tolerance. This function is useful for making
   * a nearest neighbor table from sites that land outside of the
   * primitive cell.
   */
  //***********************************************************

  template<typename CoordType> template<typename CoordType2>
  UnitCellCoord BasicStructure<CoordType>::get_unit_cell_coord(const CoordType2 &bsite, double tol) const {

    CoordType2 tsite = bsite;

    tsite.set_lattice(lattice(), CART);
    int x, y, z;

    Index b;

    b = find(tsite, tol);

    if(b == basis.size()) {
      std::cerr << "ERROR in BasicStructure::get_unit_cell_coord" << std::endl
                << "Could not find a matching basis site." << std::endl
                << "  Looking for: FRAC: " << tsite(FRAC) << "\n"
                << "               CART: " << tsite(CART) << "\n";
      exit(1);
    }

    x = round(tsite.get(0, FRAC) - basis[b].get(0, FRAC));
    y = round(tsite.get(1, FRAC) - basis[b].get(1, FRAC));
    z = round(tsite.get(2, FRAC) - basis[b].get(2, FRAC));

    return UnitCellCoord(b, x, y, z);
  };


  //*******************************************************************************************

  template<typename CoordType>
  CoordType BasicStructure<CoordType>::get_site(const UnitCellCoord &ucc) const {
    if(ucc[0] < 0 || ucc[0] >= basis.size()) {
      std::cerr << "CRITICAL ERROR: In BasicStructure<CoordType>::get_site(), UnitCellCoord " << ucc << " is out of bounds!\n"
                << "                Cannot index basis, which contains " << basis.size() << " objects.\n";
      assert(0);
      exit(1);
    }
    Coordinate trans(Vector3<double>(ucc[1], ucc[2], ucc[3]), lattice(), FRAC);
    return basis[ucc[0]] + trans;
  }

  //***********************************************************

  template<typename CoordType>
  void BasicStructure<CoordType>::set_lattice(const Lattice &new_lat, COORD_TYPE mode) {
    COORD_TYPE not_mode(CART);
    if(mode == CART) {
      not_mode = FRAC;
    }

    for(Index nb = 0; nb < basis.size(); nb++) {
      basis[nb].invalidate(not_mode);
    }

    m_lattice = new_lat;

    for(Index i = 0; i < basis.size(); i++) { //John G 121212
      basis[i].within();
    }

  }

  //\Liz D 032514
  //***********************************************************
  /**
   * Allows for the basis elements of a basic structure to be
   * manually set, e.g. as in jsonParser.cc.
   */
  //***********************************************************


  template<typename CoordType>
  void BasicStructure<CoordType>::set_basis(Array<CoordType> basis_in) {
    basis = basis_in;
    set_site_internals();
  }


  //\John G 121212
  //***********************************************************
  /**
   * Goes to a specified site of the basis and makes a flower tree
   * of pairs. It then stores the length and multiplicity of
   * the pairs in a double array, giving you a strict
   * nearest neighbor table. This version also fills up a SiteOrbitree
   * in case you want to keep it.
   * Blatantly copied from Anna's routine in old new CASM
   */
  //***********************************************************
  /*
    template<typename CoordType>
    Array<Array<Array<double> > > BasicStructure<CoordType>::get_NN_table(const double &maxr, GenericOrbitree<GenericCluster<CoordType> > &bouquet) {
      if(!bouquet.size()) {
        std::cerr << "WARNING in BasicStructure<CoordType>::get_NN_table" << std::endl;
        std::cerr << "The provided GenericOrbitree<Cluster<CoordType> > is about to be rewritten!" << std::endl;
      }

      Array<Array<Array<double> > > NN;
      GenericOrbitree<GenericCluster<CoordType> > normtree(lattice());
      GenericOrbitree<GenericCluster<CoordType> > tbouquet(lattice());
      bouquet = tbouquet;
      normtree.min_num_components = 1;
      normtree.max_num_sites = 2;
      normtree.max_length.push_back(0.0);
      normtree.max_length.push_back(0.0);
      normtree.max_length.push_back(maxr);

      normtree.generate_orbitree(*this);
      normtree.print_full_clust(std::cout);
      generate_basis_bouquet(normtree, bouquet, 2);

      Array<Array<double> > oneNN;
      oneNN.resize(2);
      for(Index i = 0; i < bouquet.size(); i++) {
        NN.push_back(oneNN);
        for(Index j = 0; j < bouquet[i].size(); j++) {
          NN[i][0].push_back(bouquet[i][j].size());
          NN[i][1].push_back(bouquet[i][j].max_length());
        }
      }
      return NN;
    }
  */
  //***********************************************************
  /**
   * Goes to a specified site of the basis and makes a flower tree
   * of pairs. It then stores the length and multiplicity of
   * the pairs in a double array, giving you a strict
   * nearest neighbor table. The bouquet used for this
   * falls into the void.
   */
  //***********************************************************
  /*
    template<typename CoordType>
    Array<Array<Array<double> > > BasicStructure<CoordType>::get_NN_table(const double &maxr) {
      GenericOrbitree<GenericCluster<CoordType> > bouquet(lattice());
      return get_NN_table(maxr, bouquet);
    }
  */

  //***********************************************************
  /**
   * Given a symmetry group, the basis of the structure will have
   * each operation applied to it. The resulting set of basis
   * from performing these operations will be averaged out,
   * yielding a new average basis that will replace the current one.
   */
  //***********************************************************

  template<typename CoordType>
  void BasicStructure<CoordType>::symmetrize(const SymGroup &relaxed_factors) {
    //First make a copy of your current basis
    //This copy will eventually become the new average basis.
    Array<CoordType> avg_basis = basis;

    //Loop through given symmetry group an fill a temporary "operated basis"
    Array<CoordType> operbasis;

    //assume identity comes first, so we skip it
    for(Index rf = 0; rf < relaxed_factors.size(); rf++) {
      operbasis.clear();
      for(Index b = 0; b < basis.size(); b++) {
        operbasis.push_back(relaxed_factors[rf]*basis[b]);
        operbasis.back().print(std::cout);
        std::cout << std::endl;
      }
      //Now that you have a transformed basis, find the closest mapping of atoms
      //Then average the distance and add it to the average basis
      for(Index b = 0; b < basis.size(); b++) {
        double smallest = 1000000;
        Coordinate bshift(lattice()), tshift(lattice());
        for(Index ob = 0; ob < operbasis.size(); ob++) {
          double dist = operbasis[ob].min_dist(basis[b], tshift);
          if(dist < smallest) {
            bshift = tshift;
            smallest = dist;
          }
        }
        bshift(CART) *= (1.0 / relaxed_factors.size());
        avg_basis[b] += bshift;
      }

    }

    return;
  }

  //***********************************************************
  /**
   * Same as the other symmetrize routine, except this one assumes
   * that the symmetry group you mean to use is the factor group
   * of your structure within a certain tolerance.
   *
   * Notice that the tolerance is also used on your point group!!
   */
  //***********************************************************


  template<typename CoordType>
  void BasicStructure<CoordType>::symmetrize(const double &tolerance) {
    SymGroup factor_group;
    generate_factor_group(factor_group, tolerance);
    symmetrize(factor_group);
    return;
  }

  //***********************************************************
  /**
   *  Call this on a structure to get new_surface_struc: the structure with a
   *  layer of vacuum added parallel to the ab plane.
   *  vacuum_thickness: thickness of vacuum layer (Angstroms)
   *  shift:  shift vector from layer to layer, assumes FRAC unless specified.
   *  The shift vector should only have values relative to a and b vectors (eg x, y, 0).
   *  Default shift is zero.
   */
  //***********************************************************

  template<typename CoordType>
  void BasicStructure<CoordType>::add_vacuum_shift(BasicStructure<CoordType> &new_surface_struc, double vacuum_thickness, Vector3<double> shift, COORD_TYPE mode) const {

    Coordinate cshift(shift, lattice(), mode);    //John G 121030
    if(!almost_zero(cshift(FRAC)[2])) {
      std::cout << cshift(FRAC) << std::endl;
      std::cout << "WARNING: You're shifting in the c direction! This will mess with your vacuum and/or structure!!" << std::endl;
      std::cout << "See BasicStructure<CoordType>::add_vacuum_shift" << std::endl;
    }

    Vector3<double> vacuum_vec;                 //unit vector perpendicular to ab plane
    vacuum_vec = lattice()[0].cross(lattice()[1]);
    vacuum_vec.normalize();
    Lattice new_lattice(lattice()[0],
                        lattice()[1],
                        lattice()[2] + vacuum_thickness * vacuum_vec + cshift(CART)); //Add vacuum and shift to c vector

    new_surface_struc = *this;
    new_surface_struc.set_lattice(new_lattice, CART);
    new_surface_struc.initialize();
    return;
  }

  //***********************************************************
  template<typename CoordType>
  void BasicStructure<CoordType>::add_vacuum_shift(BasicStructure<CoordType> &new_surface_struc, double vacuum_thickness, Coordinate shift) const {
    if(shift.get_home() != &lattice()) {
      std::cout << "WARNING: The lattice from your shift coordinate does not match the lattice of your structure!" << std::endl;
      std::cout << "See BasicStructure<CoordType>::add_vacuum_shift" << std::endl << std::endl;
    }

    add_vacuum_shift(new_surface_struc, vacuum_thickness, shift(CART), CART);
    return;
  }

  //***********************************************************
  template<typename CoordType>
  void BasicStructure<CoordType>::add_vacuum(BasicStructure<CoordType> &new_surface_struc, double vacuum_thickness) const {
    Vector3<double> shift(0, 0, 0);

    add_vacuum_shift(new_surface_struc, vacuum_thickness, shift, FRAC);

    return;
  }

  //************************************************************

  template<typename CoordType>
  void BasicStructure<CoordType>::print5(std::ostream &stream, COORD_TYPE mode, int Va_mode, char term, int prec, int pad) const {
    std::string mol_name;
    std::ostringstream num_mol_list, coord_stream;
    Array<CoordType> vacancies;

    // this statement assumes that the scaling is 1.0 always, this may needed to be changed
    stream << title << std::endl;
    lattice().print(stream);

    //declare hash
    std::map<std::string, Array<CoordType> > siteHash;
    //declare iterator for hash
    typename std::map<std::string, Array<CoordType> >::iterator it;
    //loop through all sites
    for(Index i = 0; i < basis.size(); i++) {
      CoordType tsite = basis[i];
      mol_name = tsite.occ_name();

      if(mol_name != "Va") {
        //check if mol_name is already in the hash
        it = siteHash.find(mol_name);
        if(it != siteHash.end()) {
          Array<CoordType> tarray = it-> second;
          tarray.push_back(tsite);
          siteHash[mol_name]  = tarray;
        }
        // otherwise add a new pair
        else {
          Array<CoordType> tarray;
          tarray.push_back(tsite);
          siteHash[mol_name] = tarray;
        }
      }
      //store vacancies into a separate array
      else {
        vacancies.push_back(tsite);
      }
    }
    //print names of molecules and numbers, also populate coord_stream
    it = siteHash.begin();
    stream << it -> first;
    num_mol_list << it -> second.size();
    for(Index i = 0; i < it->second.size(); i++) {
      CoordType tsite = it->second.at(i);
      tsite.Coordinate::print(coord_stream, mode, term, prec, pad);
      coord_stream << std::endl;
    }
    it++;

    for(; it != siteHash.end(); it++) {
      stream << ' ' << it-> first;
      num_mol_list << ' ' << it-> second.size();
      for(Index i = 0; i < it->second.size(); i++) {
        CoordType tsite = it->second.at(i);
        tsite.Coordinate::print(coord_stream, mode, term, prec, pad);
        coord_stream << std::endl;
      }
    }
    // add vacancies depending on the mode
    if(Va_mode == 2)
      stream << " Va";
    if(Va_mode != 0) {
      for(Index i = 0; i < vacancies.size(); i++) {
        CoordType tsite = vacancies.at(i);
        tsite.Coordinate::print(coord_stream, mode, term, prec, pad);
        coord_stream << std::endl;
      }
    }
    stream << std::endl;
    stream << num_mol_list.str() << std::endl;
    //print the COORD_TYPE
    if(mode == FRAC)
      stream << "Direct\n";
    else if(mode == CART)
      stream << "Cartesian\n";
    else
      std::cerr << "error the mode isn't defined";
    stream << coord_stream.str() << std::endl;
    return;

  }

  //************************************************************
  /// Counts sites that allow vacancies
  template<typename CoordType>
  Index BasicStructure<CoordType>::max_possible_vacancies()const {
    Index result(0);
    for(Index i = 0; i < basis.size(); i++) {
      if(basis[i].contains("Va"))
        ++result;
    }
    return result;
  }

  //************************************************************
  //read a POSCAR like file and collect all the structure variables
  //modified to read PRIM file and determine which basis to use
  //Changed by Ivy to read new VASP POSCAR format

  template<typename CoordType>
  void BasicStructure<CoordType>::read(std::istream &stream) {
    Index i;
    int t_int;
    char ch;
    Array<double> num_elem;
    Array<std::string> elem_array;
    bool read_elem = false;
    std::string tstr;
    std::stringstream tstrstream;

    CoordType tsite(lattice());

    getline(stream, title);

    m_lattice.read(stream);

    stream.ignore(100, '\n');

    //Search for Element Names
    ch = stream.peek();
    while(ch != '\n' && !stream.eof()) {
      if(isalpha(ch)) {
        read_elem = true;
        stream >> tstr;
        elem_array.push_back(tstr);
        ch = stream.peek();
      }
      else if(ch == ' ' || ch == '\t') {
        stream.ignore();
        ch = stream.peek();
      }
      else if(ch >= '0' && ch <= '9') {
        break;
      }
    }

    if(read_elem == true) {
      stream.ignore(10, '\n');
      ch = stream.peek();
    }

    //Figure out how many species
    Index num_sites = 0;
    while(ch != '\n' && !stream.eof()) {
      if(ch >= '0' && ch <= '9') {
        stream >> t_int;
        num_elem.push_back(t_int);
        num_sites += t_int;
        ch = stream.peek();
      }
      else if(ch == ' ' || ch == '\t') {
        stream.ignore();
        ch = stream.peek();
      }
      else {
        std::cerr << "Error in line 6 of structure input file. Line 6 of structure input file should contain the number of sites." << std::endl;
        exit(1);
      }
    }
    stream.get(ch);

    // fractional coordinates or cartesian
    COORD_MODE input_mode(FRAC);
    bool SD_flag(false);

    stream.get(ch);
    while(ch == ' ' || ch == '\t') {
      stream.get(ch);
    }

    if(ch == 'S' || ch == 's') {
      SD_flag = true;
      stream.ignore(1000, '\n');
      while(ch == ' ' || ch == '\t') {
        stream.get(ch);
      }
      stream.get(ch);
    }

    if(ch == 'D' || ch == 'd') {
      input_mode.set(FRAC);
    }
    else if(ch == 'C' || ch == 'c') {
      input_mode.set(CART);
    }
    else if(!SD_flag) {
      std::cerr << "Error in line 7 of structure input file. Line 7 of structure input file should specify Direct, Cartesian, or Selective Dynamics." << std::endl;
      exit(1);
    }
    else if(SD_flag) {
      std::cerr << "Error in line 8 of structure input file. Line 8 of structure input file should specify Direct or Cartesian when Selective Dynamics is on." << std::endl;
      exit(1);
    }

    stream.ignore(1000, '\n');
    //Clear basis if it is not empty
    if(basis.size() != 0) {
      std::cerr << "The structure is going to be overwritten." << std::endl;
      basis.clear();
    }

    if(read_elem) {
      int j = -1;
      Index sum_elem = 0;
      basis.reserve(num_sites);
      for(i = 0; i < num_sites; i++) {
        if(i == sum_elem) {
          j++;
          sum_elem += num_elem[j];
        }

        tsite.read(stream, elem_array[j], SD_flag);
        basis.push_back(tsite);
      }
    }
    else {
      //read the site info
      basis.reserve(num_sites);
      for(i = 0; i < num_sites; i++) {
        tsite.read(stream, SD_flag);
        basis.push_back(tsite);
      }
    }

    update();
    return;

  }

  //************************************************************
  // print structure and include all possible occupants on each site, using VASP4 format

  template<typename CoordType>
  void BasicStructure<CoordType>::print(std::ostream &stream, COORD_TYPE mode) {
    main_print(stream, mode, false, 0);
  }

  //************************************************************
  // print structure and include current occupant on each site, using VASP5 format

  template<typename CoordType>
  void BasicStructure<CoordType>::print5_occ(std::ostream &stream, COORD_TYPE mode) {
    main_print(stream, mode, true, 1);
  }

  //************************************************************
  // print structure and include current occupant on each site, using VASP4 format

  template<typename CoordType>
  void BasicStructure<CoordType>::print_occ(std::ostream &stream, COORD_TYPE mode) {
    main_print(stream, mode, false, 1);
  }

  //************************************************************
  // Private print routine called by public routines
  //   by BP, collected and modified the existing print routines (by John G?) into 1 function
  template<typename CoordType>
  void BasicStructure<CoordType>::main_print(std::ostream &stream, COORD_TYPE mode, bool version5, int option) {
    //std::cout << "begin BasicStructure<CoordType>::main_print()" << std::endl;
    // No Sorting (For now... Figure out how to do this for molecules later...)
    // If option == 0 (print all possible occupants), make sure comparing all possible occupants
    // If option == 1 (print occupying molecule name), compare just the occupant
    // If option == 2 (print all atoms of molecule), (don't do this yet)

    if(option < 0 || option > 2) {
      std::cout << "Error in BasicStructure<CoordType>::main_print()." << std::endl;
      std::cout << "  option " << option << " does not exist.  Use option = 0 or 1" << std::endl;
      std::exit(1);
    }
    else {
      if(version5 && (option == 0)) {
        std::cout << "Error in BasicStructure<CoordType>::main_print()." << std::endl;
        std::cout << "  Trying to print a BasicStructure<CoordType> with VASP version 5 format" << std::endl;
        std::cout << "  and option == 0 (print all possible occupants).  This can't be done." << std::endl;
        std::cout << "  Either use version 4 format, or option 1 (print occupying molecule name)." << std::endl;
        std::exit(1);
      }

      if(option == 2) {
        std::cout << "Error in BasicStructure<CoordType>::main_print()." << std::endl;
        std::cout << "  Trying to print all atom positions (option 2), but this is not yet coded." << std::endl;
        std::exit(1);
      }
    }

    Array<int> site_order;
    Array<int> curr_state;
    Array<std::string> site_names;

    // This is for sorting molecules by type. - NO LONGER USING THIS
    //site_order.reserve(basis.size());

    // Count up each species and their names
    // This is total for structure - NOT GOING TO SET THIS ANYMORE

    // This is consequentive molecules of same name
    // If option == 0 (print all possible occupants), make sure comparing all possible occupants
    // If option == 1 (print occupying molecule name), compare just the occupant
    // If option == 2 (print all atoms of molecule), (don't do this yet)
    Array<int> num_each_specie_for_printing;

    // if option == 0 (print all possible occupants), set current state to -1 (unknown occupant for comparison)
    //   we'll reset to the current state after counting num_each_specie_for_printing

    if(option == 0) {
      //std::cout << "  save curr state" << std::endl;
      for(Index j = 0; j < basis.size(); j++) {
        curr_state.push_back(basis[j].site_occupant().value());
        basis[j].set_occ_value(-1);
      }
    }
    // if option == 1 (print current occupants), check that current state is not -1 (unknown occupant)
    if(option == 1) {
      //std::cout << "  check curr state" << std::endl;
      for(Index j = 0; j < basis.size(); j++) {
        if(basis[j].site_occupant().value() == -1) {
          std::cout << "Error in BasicStructure<CoordType>::main_print() using option 1 (print occupying molecule name)." << std::endl;
          std::cout << "  basis " << j << " occupant state is unknown." << std::endl;
          std::exit(1);
        }
      }
    }


    //std::cout << "  get num each specie for printing" << std::endl;

    for(Index i = 0; i < basis.size(); i++) {
      if(option == 0) { //(print all possible occupants)
        if(i == 0)
          num_each_specie_for_printing.push_back(1);
        else if(basis[i - 1].site_occupant() == basis[i].site_occupant())
          num_each_specie_for_printing.back()++;
        else
          num_each_specie_for_printing.push_back(1);
      }
      else if(option == 1) {   //(print all occupying molecule)
        if(basis[i].occ_name() == "Va") {
          continue;
        }

        if(i == 0) {
          num_each_specie_for_printing.push_back(1);
          site_names.push_back(basis[i].occ_name());
        }
        else if(basis[i - 1].occ_name() == basis[i].occ_name())
          num_each_specie_for_printing.back()++;
        else {
          num_each_specie_for_printing.push_back(1);
          site_names.push_back(basis[i].occ_name());
        }

      }
    }

    // if option == 0 (print all possible occupants),
    //   reset the current state
    //std::cout << "  reset curr state" << std::endl;

    if(option == 0) {
      for(Index j = 0; j < basis.size(); j++)
        basis[j].set_occ_value(curr_state[j]);
    }

    stream << title << '\n';

    // Output lattice: scaling factor and lattice vectors
    //std::cout << "  print lattice" << std::endl;
    lattice().print(stream);

    // Output species names
    //std::cout << "  print specie names" << std::endl;
    if(version5) {
      for(Index i = 0; i < site_names.size(); i++) {
        stream << " " << site_names[i];
      }
      stream << std::endl;
    }


    // Output species counts
    //std::cout << "  print specie counts" << std::endl;
    for(Index i = 0; i < num_each_specie_for_printing.size(); i++) {
      stream << " " << num_each_specie_for_printing[i];
    }
    stream << std::endl;



    COORD_MODE output_mode(mode);

    stream << COORD_MODE::NAME() << '\n';

    // Output coordinates
    //std::cout << "  print coords" << std::endl;

    for(Index i = 0; i < basis.size(); i++) {
      basis[i].print(stream);
      stream << '\n';
    }
    stream << std::flush;

    //std::cout << "finish BasicStructure<CoordType>::main_print()" << std::endl;

    return;
  }

  //***********************************************************

  template<typename CoordType>
  void BasicStructure<CoordType>::print_xyz(std::ostream &stream) {
    stream << basis.size() << '\n';
    stream << title << '\n';
    stream.precision(7);
    stream.width(11);
    stream.flags(std::ios::showpoint | std::ios::fixed | std::ios::right);

    for(Index i = 0; i < basis.size(); i++) {
      stream << std::setw(2) << basis[i].occ_name() << " ";
      stream << std::setw(12) << basis[i](CART) << '\n';
    }

  }

  //***********************************************************

  template<typename CoordType>
  BasicStructure<CoordType> &BasicStructure<CoordType>::operator+=(const Coordinate &shift) {

    for(Index i = 0; i < basis.size(); i++) {
      basis[i] += shift;
    }

    //factor_group += shift;
    //asym_unit += shift;
    return (*this);
  }


  //***********************************************************

  template<typename CoordType>
  BasicStructure<CoordType> &BasicStructure<CoordType>::operator-=(const Coordinate &shift) {

    for(Index i = 0; i < basis.size(); i++) {
      basis[i] -= shift;
    }
    //factor_group -= shift;
    //asym_unit -= shift;
    return (*this);
  }

  //***********************************************************

  template<typename CoordType>
  BasicStructure<CoordType> operator*(const SymOp &LHS, const BasicStructure<CoordType> &RHS) { //AAB

    return BasicStructure<CoordType>(RHS).apply_sym(LHS);
  }

  //***********************************************************

  template<typename CoordType>
  BasicStructure<CoordType> operator*(const Lattice &LHS, const BasicStructure<CoordType> &RHS) {
    BasicStructure<CoordType> tsuper(LHS);
    tsuper.fill_supercell(RHS);
    return tsuper;
  }

  //****************************************************

  template<typename CoordType>
  jsonParser &BasicStructure<CoordType>::to_json(jsonParser &json) const {
    json.put_obj();

    // std::string title;
    json["title"] = title;

    // Lattice lattice;
    json["lattice"] = lattice();

    // Array<CoordType> basis;
    json["basis"] = basis;

    return json;
  }

  //****************************************************

  // Assumes constructor CoordType::CoordType(Lattice) exists
  template<typename CoordType>
  void BasicStructure<CoordType>::from_json(const jsonParser &json) {
    try {

      // std::string title;
      CASM::from_json(title, json["title"]);

      // Lattice lattice;
      CASM::from_json(m_lattice, json["lattice"]);

      // Array<CoordType> basis;
      basis.clear();
      CoordType coordtype(lattice());
      for(int i = 0; i < json["basis"].size(); i++) {
        CASM::from_json(coordtype, json["basis"][i]);
        basis.push_back(coordtype);
      }

    }
    catch(...) {
      /// re-throw exceptions
      throw;
    }

  }

  //****************************************************

  template<typename CoordType>
  jsonParser &to_json(const BasicStructure<CoordType> &basic, jsonParser &json) {
    return basic.to_json(json);
  }

  // Assumes constructor CoordType::CoordType(Lattice) exists
  template<typename CoordType>
  void from_json(BasicStructure<CoordType> &basic, const jsonParser &json) {
    basic.from_json(json);
  }


}

