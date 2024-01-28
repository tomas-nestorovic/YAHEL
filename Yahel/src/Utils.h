#ifndef UTILS_H
#define UTILS_H

namespace Yahel{
namespace Utils{

	template<typename T>
	class CVarBackup{
		const T value0;
	protected:
		T &var;
	public:
		inline CVarBackup(T &var)
			: value0(var) , var(var) {
		}
		inline ~CVarBackup(){
			var=value0;
		}
		inline operator const T &() const{
			return value0;
		}
	};

	template<typename T>
	class CVarTempReset:public CVarBackup<T>{
	public:
		inline CVarTempReset(T &var,const T &newValue)
			: CVarBackup(var) {
			var=newValue;
		}
		inline operator T() const{ return var; }
	};

	class CExclusivelyLocked sealed{
		CRITICAL_SECTION &cs;
	public:
		CExclusivelyLocked(CRITICAL_SECTION &cs);
		~CExclusivelyLocked();
	};

	class CYahelPen:public LOGPEN{
		const HPEN handle;
	public:
		CYahelPen(BYTE thickness,COLORREF color);
		~CYahelPen();

		inline operator HPEN() const{ return handle; }
	};

	class CYahelBrush:public LOGBRUSH{
		HBRUSH handle;
	public:
		static const CYahelBrush None,Black,White;

		CYahelBrush(int stockObjectId);
		CYahelBrush(COLORREF solidColor,bool sysColor);
		~CYahelBrush();

		inline operator HBRUSH() const{ return handle; }
		inline operator COLORREF() const{ return lbColor; }
		CYahelBrush &operator=(CYahelBrush &&r);
	};

	class CYahelFont{
		const HFONT handle;
		int charAvgWidth,charHeight;
	public:
		struct TLogFont:public LOGFONTW{
			TLogFont(LPCWSTR face,int pointHeight);
			TLogFont(HFONT hFont);
		};

		CYahelFont(const LOGFONTW &lf);
		~CYahelFont();

		inline operator HFONT() const{ return handle; }

		inline int GetCharAvgWidth() const{ return charAvgWidth; }
		inline int GetCharHeight() const{ return charHeight; }
		SIZE GetTextSize(LPCTSTR text,int textLength) const;
		SIZE GetTextSize(LPCTSTR text) const;
	};

	class CYahelContextMenu{
		const HMENU hResourceMenu;
		const HMENU handle;
	public:
		CYahelContextMenu(UINT idMenuRes);
		~CYahelContextMenu();

		inline operator HMENU() const{ return handle; }

		void UpdateUi(const IOwner &owner) const;
	};

	class CYahelDialog{
		static INT_PTR WINAPI DialogProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam);
	protected:
		HWND hDlg;

		HWND GetDlgItemHwnd(WORD id) const{ return ::GetDlgItem(hDlg,id); }
		bool EnableDlgItem(WORD id,bool enabled=true) const;
		bool EnableDlgItems(PCWORD pIds,bool enabled) const;
		inline bool IsDlgButtonChecked(WORD id) const{ return ::IsDlgButtonChecked(hDlg,id)!=BST_UNCHECKED; }
		inline void CheckDlgButton(WORD id,bool checked) const{ ::CheckDlgButton(hDlg,id,checked*BST_CHECKED); }
		void InitDlgItemTextW(WORD id,...) const;
		inline int GetDlgItemText(WORD id,PTCHAR buf,UINT bufLength) const{ return ::GetDlgItemText( hDlg, id, buf, bufLength ); }
		inline int GetDlgItemTextW(WORD id,PWCHAR buf,UINT bufLength) const{ return ::GetDlgItemTextW( hDlg, id, buf, bufLength ); }
		int GetDlgItemTextLength(WORD id) const;
		inline int SetDlgItemText(WORD id,LPCTSTR text) const{ return ::SetDlgItemText( hDlg, id, text ); }
		inline int SetDlgItemTextW(WORD id,LPCWSTR text) const{ return ::SetDlgItemTextW( hDlg, id, text ); }
		inline int GetDlgItemInt(WORD id) const{ return ::GetDlgItemInt( hDlg, id, nullptr, TRUE ); }
		inline int SetDlgItemInt(WORD id,int i) const{ return ::SetDlgItemInt( hDlg, id, i, TRUE ); }
		HWND FocusDlgItem(WORD id) const;
		inline LRESULT SendCommand(WPARAM wParam,LPARAM lParam=0) const{ return ::SendMessage( hDlg, WM_COMMAND, wParam, lParam ); }
		RECT MapDlgItemClientRect(WORD id) const;
		virtual bool InitDialog()=0;
		virtual bool ValidateDialog()=0;
		virtual bool OnCommand(WPARAM wParam,LPARAM lParam)=0;

		CYahelDialog();
	public:
		UINT_PTR DoModal(UINT nIDTemplate,HWND hParent=0);
	};

	class CViewportOrg{
		const HDC dc;
		POINT pt0;
	public:
		CViewportOrg(HDC dc,TRow r,TCol c,const CYahelFont &font);
		~CViewportOrg();
	};

	class CSingleNumberDialog sealed:public CYahelDialog{
		const LPCTSTR caption,label;
		const TPosInterval range;
		bool hexa;

		bool InitDialog() override;
		bool ValidateDialog() override;
		bool OnCommand(WPARAM wParam,LPARAM lParam) override;
		bool GetCurrentValue(TPosition &outValue) const;
	public:
		TPosition Value;

		CSingleNumberDialog(LPCTSTR caption,LPCTSTR label,const TPosInterval &range,TPosition initValue,bool hexa);

		inline UINT_PTR DoModal(HWND hParent=0){ return __super::DoModal(IDR_YAHEL_SINGLE_NUMBER,hParent); }
	};



	POINT MakePoint(LPARAM lParam);
	COLORREF GetSaturatedColor(COLORREF color,float saturationFactor);
	COLORREF GetBlendedColor(COLORREF color1,COLORREF color2,float blendFactor=.5f);
	void RandomizeData(PVOID buffer,TPosition nBytes);

}
}

#endif // UTILS_H
