/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#ifndef _MITK_M2_TYPE_SELECTION_H
#define _MITK_M2_TYPE_SELECTION_H

#include <berryIStructuredSelection.h>

namespace m2
{
  template <class ObjectType>
  class SelectionObjectT : public berry::Object
  {
  public:
    berryObjectMacro(SelectionObjectT);

	inline SelectionObjectT() : m_Object() {}
	inline SelectionObjectT(const ObjectType &selection) : m_Object(selection) {}

	inline const ObjectType &GetContent() const { return m_Object; }

	inline bool operator==(const berry::Object *obj) const override
    {
      if (const SelectionObjectT *other = dynamic_cast<const SelectionObjectT *>(obj))
      {
        return m_Object == other->GetContent();
      }

      return false;
    }

  private:
    ObjectType m_Object;
  };

  /**
   * \brief Used by plugins to communicate selections ion image references
   *
   * For example used by the ions view to inform about the currently selected ions.
   */
  template <class ObjectType>
  class SelectionT : public virtual berry::IStructuredSelection
  {
  public:
    berryObjectMacro(SelectionT);
    using SelectionObjectType = SelectionObjectT<ObjectType>;
    using VectorObjectType = std::vector<ObjectType>;

	inline SelectionT() : m_Selection(new ContainerType()) {}

	inline SelectionT(const ObjectType &info) : m_Selection(new ContainerType())
    {
      typename SelectionObjectType::Pointer obj(new SelectionObjectType(info));
      m_Selection->push_back(obj);
    }

    inline SelectionT(const VectorObjectType &infos) : m_Selection(new ContainerType())
    {
      for (typename VectorObjectType::const_iterator i = infos.begin(); i != infos.end(); ++i)
      {
        typename SelectionObjectType::Pointer obj(new SelectionObjectType(*i));
        m_Selection->push_back(obj);
      }
    }

    inline Object::Pointer GetFirstElement() const override
    {
      if (m_Selection->empty())
        return berry::Object::Pointer();

      return *(m_Selection->begin());
    }

	inline iterator Begin() const override { return m_Selection->begin(); }
	inline iterator End() const override { return m_Selection->end(); }
	inline int Size() const override { return m_Selection->size(); }

	inline ContainerType::Pointer ToVector() const override { return m_Selection; }

	inline VectorObjectType GetSelectedObjects() const
    {
      VectorObjectType selectedInfos;
      if (IsEmpty())
        return selectedInfos;

      typename SelectionObjectType::Pointer infoObject;
      //ObjectType info;

      for (iterator it = Begin(); it != End(); ++it)
      {
        infoObject = it->Cast<SelectionObjectType>();
        if (infoObject.IsNotNull())
        {
          selectedInfos.push_back(infoObject->GetContent());
        }
      }
      return selectedInfos;
    }

    /**
     * @see berry::ISelection::IsEmpty()
     */
	inline bool IsEmpty() const override { return m_Selection->empty(); }

	inline bool operator==(const berry::Object *obj) const override
    {
      if (const berry::IStructuredSelection *other = dynamic_cast<const berry::IStructuredSelection *>(obj))
      {
        return m_Selection == other->ToVector();
      }

      return false;
    }

    /**
     * \brief berry wrapper for a Ions selection info
     *
     * Used by m2::IonsInfoSelection.
     */

  protected:
    ContainerType::Pointer m_Selection;
  };


} // namespace m2


#endif
