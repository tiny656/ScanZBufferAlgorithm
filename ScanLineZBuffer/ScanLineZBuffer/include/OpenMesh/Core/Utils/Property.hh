/*===========================================================================*\
 *                                                                           *
 *                               OpenMesh                                    *
 *      Copyright (C) 2001-2015 by Computer Graphics Group, RWTH Aachen      *
 *                           www.openmesh.org                                *
 *                                                                           *
 *---------------------------------------------------------------------------* 
 *  This file is part of OpenMesh.                                           *
 *                                                                           *
 *  OpenMesh is free software: you can redistribute it and/or modify         * 
 *  it under the terms of the GNU Lesser General Public License as           *
 *  published by the Free Software Foundation, either version 3 of           *
 *  the License, or (at your option) any later version with the              *
 *  following exceptions:                                                    *
 *                                                                           *
 *  If other files instantiate templates or use macros                       *
 *  or inline functions from this file, or you compile this file and         *
 *  link it with other files to produce an executable, this file does        *
 *  not by itself cause the resulting executable to be covered by the        *
 *  GNU Lesser General Public License. This exception does not however       *
 *  invalidate any other reasons why the executable file might be            *
 *  covered by the GNU Lesser General Public License.                        *
 *                                                                           *
 *  OpenMesh is distributed in the hope that it will be useful,              *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU Lesser General Public License for more details.                      *
 *                                                                           *
 *  You should have received a copy of the GNU LesserGeneral Public          *
 *  License along with OpenMesh.  If not,                                    *
 *  see <http://www.gnu.org/licenses/>.                                      *
 *                                                                           *
\*===========================================================================*/ 

/*===========================================================================*\
 *                                                                           *             
 *   $Revision: 1196 $                                                         *
 *   $Date: 2015-01-14 16:41:22 +0100 (Wed, 14 Jan 2015) $                   *
 *                                                                           *
\*===========================================================================*/

#ifndef OPENMESH_PROPERTY_HH
#define OPENMESH_PROPERTY_HH


//== INCLUDES =================================================================


#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Mesh/Handles.hh>
#include <OpenMesh/Core/Utils/BaseProperty.hh>
#include <vector>
#include <string>
#include <algorithm>


//== NAMESPACES ===============================================================

namespace OpenMesh {

//== CLASS DEFINITION =========================================================

/** \class PropertyT Property.hh <OpenMesh/Core/Utils/PropertyT.hh>
 *
 *  \brief Default property class for any type T.
 *
 *  The default property class for any type T.
 *
 *  The property supports persistency if T is a "fundamental" type:
 *  - integer fundamental types except bool:
 *    char, short, int, long, long long (__int64 for MS VC++) and
 *    their unsigned companions.
 *  - float fundamentals except <tt>long double</tt>:
 *    float, double
 *  - %OpenMesh vector types
 *
 *  Persistency of non-fundamental types is supported if and only if a
 *  specialization of struct IO::binary<> exists for the wanted type.
 */

// TODO: it might be possible to define Property using kind of a runtime info
// structure holding the size of T. Then reserve, swap, resize, etc can be written
// in pure malloc() style w/o virtual overhead. Template member function proved per
// element access to the properties, asserting dynamic_casts in debug

template <class T>
class PropertyT : public BaseProperty
{
public:

  typedef T                                       Value;
  typedef std::vector<T>                          vector_type;
  typedef T                                       value_type;
  typedef typename vector_type::reference         reference;
  typedef typename vector_type::const_reference   const_reference;

public:

  /// Default constructor
  PropertyT(const std::string& _name = "<unknown>")
  : BaseProperty(_name)
  {}

  /// Copy constructor
  PropertyT(const PropertyT & _rhs)
      : BaseProperty( _rhs ), data_( _rhs.data_ ) {}

public: // inherited from BaseProperty

  virtual void reserve(size_t _n) { data_.reserve(_n);    }
  virtual void resize(size_t _n)  { data_.resize(_n);     }
  virtual void clear()  { data_.clear(); vector_type().swap(data_);    }
  virtual void push_back()        { data_.push_back(T()); }
  virtual void swap(size_t _i0, size_t _i1)
  { std::swap(data_[_i0], data_[_i1]); }
  virtual void copy(size_t _i0, size_t _i1)
  { data_[_i1] = data_[_i0]; }

public:

  virtual void set_persistent( bool _yn )
  { check_and_set_persistent<T>( _yn ); }

  virtual size_t       n_elements()   const { return data_.size(); }
  virtual size_t       element_size() const { return IO::size_of<T>(); }

#ifndef DOXY_IGNORE_THIS
  struct plus {
    size_t operator () ( size_t _b, const T& _v )
    { return _b + IO::size_of<T>(_v); }
  };
#endif

  virtual size_t size_of(void) const
  {
    if (element_size() != IO::UnknownSize)
      return this->BaseProperty::size_of(n_elements());
    return std::accumulate(data_.begin(), data_.end(), size_t(0), plus());
  }

  virtual size_t size_of(size_t _n_elem) const
  { return this->BaseProperty::size_of(_n_elem); }

  virtual size_t store( std::ostream& _ostr, bool _swap ) const
  {
    if ( IO::is_streamable<vector_type>() )
      return IO::store(_ostr, data_, _swap );
    size_t bytes = 0;
    for (size_t i=0; i<n_elements(); ++i)
      bytes += IO::store( _ostr, data_[i], _swap );
    return bytes;
  }

  virtual size_t restore( std::istream& _istr, bool _swap )
  {
    if ( IO::is_streamable<vector_type>() )
      return IO::restore(_istr, data_, _swap );
    size_t bytes = 0;
    for (size_t i=0; i<n_elements(); ++i)
      bytes += IO::restore( _istr, data_[i], _swap );
    return bytes;
  }

public: // data access interface

  /// Get pointer to array (does not work for T==bool)
  const T* data() const {

    if( data_.empty() )
      return 0;

    return &data_[0];
  }

  /// Get reference to property vector (be careful, improper usage, e.g. resizing, may crash OpenMesh!!!)
  vector_type& data_vector() {
    return data_;
  }

  /// Const access to property vector
  const vector_type& data_vector() const {
    return data_;
  }

  /// Access the i'th element. No range check is performed!
  reference operator[](int _idx)
  {
    assert( size_t(_idx) < data_.size() );
    return data_[_idx];
  }

  /// Const access to the i'th element. No range check is performed!
  const_reference operator[](int _idx) const
  {
    assert( size_t(_idx) < data_.size());
    return data_[_idx];
  }

  /// Make a copy of self.
  PropertyT<T>* clone() const
  {
    PropertyT<T>* p = new PropertyT<T>( *this );
    return p;
  }


private:

  vector_type data_;
};

//-----------------------------------------------------------------------------


/** Property specialization for bool type. 

  The data will be stored as a bitset.
 */
template <>
class PropertyT<bool> : public BaseProperty
{
public:

  typedef std::vector<bool>                       vector_type;
  typedef bool                                    value_type;
  typedef vector_type::reference                  reference;
  typedef vector_type::const_reference            const_reference;

public:

  PropertyT(const std::string& _name = "<unknown>")
    : BaseProperty(_name)
  { }

public: // inherited from BaseProperty

  virtual void reserve(size_t _n) { data_.reserve(_n);    }
  virtual void resize(size_t _n)  { data_.resize(_n);     }
  virtual void clear()  { data_.clear(); vector_type().swap(data_);    }
  virtual void push_back()        { data_.push_back(bool()); }
  virtual void swap(size_t _i0, size_t _i1)
  { bool t(data_[_i0]); data_[_i0]=data_[_i1]; data_[_i1]=t; }
  virtual void copy(size_t _i0, size_t _i1)
  { data_[_i1] = data_[_i0]; }

public:

  virtual void set_persistent( bool _yn )
  {
    check_and_set_persistent<bool>( _yn );
  }

  virtual size_t       n_elements()   const { return data_.size();  }
  virtual size_t       element_size() const { return UnknownSize;    }
  virtual size_t       size_of() const      { return size_of( n_elements() ); }
  virtual size_t       size_of(size_t _n_elem) const
  {
    return _n_elem / 8 + ((_n_elem % 8)!=0);
  }

  size_t store( std::ostream& _ostr, bool /* _swap */ ) const
  {
    size_t bytes = 0;

    size_t N = data_.size() / 8;
    size_t R = data_.size() % 8;

    size_t        idx;  // element index
    size_t        bidx;
    unsigned char bits; // bitset

    for (bidx=idx=0; idx < N; ++idx, bidx+=8)
    {
      bits = !!data_[bidx]
        | (!!data_[bidx+1] << 1)
        | (!!data_[bidx+2] << 2)
        | (!!data_[bidx+3] << 3)
        | (!!data_[bidx+4] << 4)
        | (!!data_[bidx+5] << 5)
        | (!!data_[bidx+6] << 6)
        | (!!data_[bidx+7] << 7);
      _ostr << bits;
    }
    bytes = N;

    if (R)
    {
      bits = 0;
      for (idx=0; idx < R; ++idx)
        bits |= !!data_[bidx+idx] << idx;
      _ostr << bits;
      ++bytes;
    }

    std::cout << std::endl;

    assert( bytes == size_of() );

    return bytes;
  }

  size_t restore( std::istream& _istr, bool /* _swap */ )
  {
    size_t bytes = 0;

    size_t N = data_.size() / 8;
    size_t R = data_.size() % 8;

    size_t        idx;  // element index
    size_t        bidx; //
    unsigned char bits; // bitset

    for (bidx=idx=0; idx < N; ++idx, bidx+=8)
    {
      _istr >> bits;
      data_[bidx+0] = !!(bits & 0x01);
      data_[bidx+1] = !!(bits & 0x02);
      data_[bidx+2] = !!(bits & 0x04);
      data_[bidx+3] = !!(bits & 0x08);
      data_[bidx+4] = !!(bits & 0x10);
      data_[bidx+5] = !!(bits & 0x20);
      data_[bidx+6] = !!(bits & 0x40);
      data_[bidx+7] = !!(bits & 0x80);
    }
    bytes = N;

    if (R)
    {
      _istr >> bits;
      for (idx=0; idx < R; ++idx)
        data_[bidx+idx] = !!(bits & (1<<idx));
      ++bytes;
    }

    std::cout << std::endl;

    return bytes;
  }


public:

  /// Get reference to property vector (be careful, improper usage, e.g. resizing, may crash OpenMesh!!!)
  vector_type& data_vector() {
    return data_;
  }

  /// Const access to property vector
  const vector_type& data_vector() const {
    return data_;
  }

  /// Access the i'th element. No range check is performed!
  reference operator[](int _idx)
  {
    assert( size_t(_idx) < data_.size() );
    return data_[_idx];
  }

  /// Const access to the i'th element. No range check is performed!
  const_reference operator[](int _idx) const
  {
    assert( size_t(_idx) < data_.size());
    return data_[_idx];
  }

  /// Make a copy of self.
  PropertyT<bool>* clone() const
  {
    PropertyT<bool>* p = new PropertyT<bool>( *this );
    return p;
  }


private:

  vector_type data_;
};


//-----------------------------------------------------------------------------


/** Property specialization for std::string type.
*/
template <>
class PropertyT<std::string> : public BaseProperty
{
public:

  typedef std::string                             Value;
  typedef std::vector<std::string>                vector_type;
  typedef std::string                             value_type;
  typedef vector_type::reference                  reference;
  typedef vector_type::const_reference            const_reference;

public:

  PropertyT(const std::string& _name = "<unknown>")
    : BaseProperty(_name)
  { }

public: // inherited from BaseProperty

  virtual void reserve(size_t _n) { data_.reserve(_n);    }
  virtual void resize(size_t _n)  { data_.resize(_n);     }
  virtual void clear()  { data_.clear(); vector_type().swap(data_);    }
  virtual void push_back()        { data_.push_back(std::string()); }
  virtual void swap(size_t _i0, size_t _i1) {
    std::swap(data_[_i0], data_[_i1]);
  }
  virtual void copy(size_t _i0, size_t _i1)
  { data_[_i1] = data_[_i0]; }

public:

  virtual void set_persistent( bool _yn )
  { check_and_set_persistent<std::string>( _yn ); }

  virtual size_t       n_elements()   const { return data_.size();  }
  virtual size_t       element_size() const { return UnknownSize;    }
  virtual size_t       size_of() const
  { return IO::size_of( data_ ); }

  virtual size_t       size_of(size_t /* _n_elem */) const
  { return UnknownSize; }

  /// Store self as one binary block. Max. length of a string is 65535 bytes.
  size_t store( std::ostream& _ostr, bool _swap ) const
  { return IO::store( _ostr, data_, _swap ); }

  size_t restore( std::istream& _istr, bool _swap )
  { return IO::restore( _istr, data_, _swap ); }

public:

  const value_type* data() const {
      if( data_.empty() )
	  return 0;

      return (value_type*) &data_[0];
  }

  /// Access the i'th element. No range check is performed!
  reference operator[](int _idx) {
    assert( size_t(_idx) < data_.size());
    return ((value_type*) &data_[0])[_idx];
  }

  /// Const access the i'th element. No range check is performed!
  const_reference operator[](int _idx) const {
    assert( size_t(_idx) < data_.size());
    return ((value_type*) &data_[0])[_idx];
  }

  PropertyT<value_type>* clone() const {
    PropertyT<value_type>* p = new PropertyT<value_type>( *this );
    return p;
  }
private:

  vector_type data_;

};

/// Base property handle.
template <class T>
struct BasePropHandleT : public BaseHandle
{
  typedef T                                       Value;
  typedef std::vector<T>                          vector_type;
  typedef T                                       value_type;
  typedef typename vector_type::reference         reference;
  typedef typename vector_type::const_reference   const_reference;

  explicit BasePropHandleT(int _idx=-1) : BaseHandle(_idx) {}
};


/** \ingroup mesh_property_handle_group
 *  Handle representing a vertex property
 */
template <class T>
struct VPropHandleT : public BasePropHandleT<T>
{
  typedef T                       Value;
  typedef T                       value_type;

  explicit VPropHandleT(int _idx=-1) : BasePropHandleT<T>(_idx) {}
  explicit VPropHandleT(const BasePropHandleT<T>& _b) : BasePropHandleT<T>(_b) {}
};


/** \ingroup mesh_property_handle_group
 *  Handle representing a halfedge property
 */
template <class T>
struct HPropHandleT : public BasePropHandleT<T>
{
  typedef T                       Value;
  typedef T                       value_type;

  explicit HPropHandleT(int _idx=-1) : BasePropHandleT<T>(_idx) {}
  explicit HPropHandleT(const BasePropHandleT<T>& _b) : BasePropHandleT<T>(_b) {}
};


/** \ingroup mesh_property_handle_group
 *  Handle representing an edge property
 */
template <class T>
struct EPropHandleT : public BasePropHandleT<T>
{
  typedef T                       Value;
  typedef T                       value_type;

  explicit EPropHandleT(int _idx=-1) : BasePropHandleT<T>(_idx) {}
  explicit EPropHandleT(const BasePropHandleT<T>& _b) : BasePropHandleT<T>(_b) {}
};


/** \ingroup mesh_property_handle_group
 *  Handle representing a face property
 */
template <class T>
struct FPropHandleT : public BasePropHandleT<T>
{
  typedef T                       Value;
  typedef T                       value_type;

  explicit FPropHandleT(int _idx=-1) : BasePropHandleT<T>(_idx) {}
  explicit FPropHandleT(const BasePropHandleT<T>& _b) : BasePropHandleT<T>(_b) {}
};


/** \ingroup mesh_property_handle_group
 *  Handle representing a mesh property
 */
template <class T>
struct MPropHandleT : public BasePropHandleT<T>
{
  typedef T                       Value;
  typedef T                       value_type;

  explicit MPropHandleT(int _idx=-1) : BasePropHandleT<T>(_idx) {}
  explicit MPropHandleT(const BasePropHandleT<T>& _b) : BasePropHandleT<T>(_b) {}
};

} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_PROPERTY_HH defined
//=============================================================================
