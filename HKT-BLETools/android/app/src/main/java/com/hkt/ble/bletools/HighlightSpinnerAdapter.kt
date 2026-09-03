package com.hkt.ble.bletools

import android.content.Context
import android.view.View
import android.view.ViewGroup
import android.widget.ArrayAdapter
import android.widget.Spinner
import android.widget.TextView
import androidx.core.content.ContextCompat

class HighlightSpinnerAdapter(
    context: Context,
    private val items: List<String>,
    private val spinner: Spinner
) : ArrayAdapter<String>(context, android.R.layout.simple_spinner_item, items) {

    init {
        setDropDownViewResource(R.layout.spinner_dropdown_item_highlight)
    }

    override fun getDropDownView(position: Int, convertView: View?, parent: ViewGroup): View {
        val view = super.getDropDownView(position, convertView, parent)
        val textView = view as TextView
        if (position == spinner.selectedItemPosition) {
            textView.setTextColor(ContextCompat.getColor(context, R.color.colorPrimary))
        } else {
            textView.setTextColor(ContextCompat.getColor(context, R.color.black))
        }
        return view
    }
}
