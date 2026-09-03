//package com.hkt.ble.bletools
//
//public class MultiSpinner extends TextView implements View.OnClickListener,DialogInterface.OnClickListener{
//
//    private ListView listView;
//
//    private Context context;
//
//    private String title;
//
//    private List < SimpleSpinnerOption > dataList;
//
//    private Adapter adapter;
//
//    private Set < Object > checkedSet;
//
//    private int selectCount = -1;
//
//    private boolean isEmpty() {
//        return dataList == null?true:dataList.isEmpty();
//    }
//
//    public String getTitle() {
//        return title;
//    }
//
//    public void setTitle(String title) {
//        this.title = title;
//    }
//
//    public void setCheckedSet(Set<Object> checkedSet) {
//        this.checkedSet = checkedSet;
//        showSelectedContent();
//    }
//
//    public List < SimpleSpinnerOption > getDataList () {
//        return dataList;
//    }
//
//    public int getSelectCount() {
//        return selectCount;
//    }
//
//    public void setSelectCount(int selectCount) {
//        this.selectCount = selectCount;
//    }
//
//    public void setDataList(List<SimpleSpinnerOption> dataList) {
//        this.dataList = dataList;
//        if (adapter == null) {
//            adapter = new Adapter (dataList);
//            this.listView.setAdapter(adapter);
//        } else {
//            adapter.setList(dataList);
//            adapter.notifyDataSetChanged();
//        }
//    }
//
//    public MultiSpinner (Context context) {
//        super(context, null);
//    }
//
//    public MultiSpinner (Context context, AttributeSet attrs) {
//        super(context, attrs);
//        this.context = context;
//        this.setOnClickListener(this);
//        listView = new ListView (context);
//        listView.setLayoutParams(
//            new LinearLayout . LayoutParams (ViewGroup.LayoutParams.MATCH_PARENT,
//            ViewGroup.LayoutParams.MATCH_PARENT
//        ));
//        adapter = new Adapter (null);
//        this.listView.setAdapter(adapter);
//    }
//
//    public MultiSpinner (Context context, AttributeSet attrs, int defStyleAttr) {
//        super(context, attrs, defStyleAttr);
//    }
//}