#include "callbacks_dtools.h"

/* ***** ***** IO REGISTERS ***** ***** */
static int cpu=0;
static u32 address;
static BOOL hword;

static GtkLabel * reg_address;
static GtkEntry * reg_value;

void display_current_reg() {
	char text_address[16];
	char text_value[16];
	char * patt = "0x%08X";
	u32 w = MMU_read32(cpu,address);

	if (hword) { patt = "0x%04X"; w &= 0xFFFF; }
	sprintf(text_address, "0x%08X", address);
	sprintf(text_value,   patt, w);
	gtk_label_set_text(reg_address, text_address);
	gtk_entry_set_text(reg_value,   text_value);
}

void display_reg(u32 address_, BOOL hword_) {
	address = address_;
	hword = hword_;
	display_current_reg();
}


void on_wtools_1_combo_cpu_changed    (GtkComboBox *widget, gpointer user_data) {
	/* c == 0 means ARM9 */
	cpu=gtk_combo_box_get_active(widget);
	display_current_reg();
}

void on_wtools_1_IOregs_show          (GtkWidget *widget, gpointer user_data) {
	GtkWidget * b = glade_xml_get_widget(xml_tools, "wtools_1_r_ime");
	GtkWidget * combo = glade_xml_get_widget(xml_tools, "wtools_1_combo_cpu");
	reg_address = (GtkLabel*)glade_xml_get_widget(xml_tools, "wtools_1_REGADRESS");
	reg_value = (GtkEntry*)glade_xml_get_widget(xml_tools, "wtools_1_REGVALUE");
	// do as if we had selected this button and ARM7 cpu
	gtk_toggle_button_set_active((GtkToggleButton*)b, TRUE);
	gtk_combo_box_set_active((GtkComboBox*)combo, 0);
}

void on_wtools_1_r_ipcfifocnt_toggled (GtkToggleButton *togglebutton, gpointer user_data) { display_reg(REG_IPCFIFOCNT,TRUE); }
void on_wtools_1_r_spicnt_toggled     (GtkToggleButton *togglebutton, gpointer user_data) { display_reg(REG_SPICNT,TRUE); }
void on_wtools_1_r_ime_toggled        (GtkToggleButton *togglebutton, gpointer user_data) { display_reg(REG_IME,TRUE); }
void on_wtools_1_r_ie_toggled         (GtkToggleButton *togglebutton, gpointer user_data) { display_reg(REG_IE,FALSE); }
void on_wtools_1_r_if_toggled         (GtkToggleButton *togglebutton, gpointer user_data) { display_reg(REG_IF,FALSE); }
void on_wtools_1_r_power_cr_toggled   (GtkToggleButton *togglebutton, gpointer user_data) { display_reg(REG_POWCNT1,TRUE); }