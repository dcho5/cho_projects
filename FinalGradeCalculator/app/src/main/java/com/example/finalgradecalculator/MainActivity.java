package com.example.finalgradecalculator;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import org.w3c.dom.Text;

public class MainActivity extends AppCompatActivity {

    private EditText c;
    private EditText g;
    private EditText w;
    private Button calcButton;
    private TextView result;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        c = findViewById(R.id.Current);
        g = findViewById(R.id.Goal);
        w = findViewById(R.id.Weight);

        calcButton = findViewById(R.id.calculate);
        result = findViewById(R.id.result);

        calcButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (c.getText().toString().equals("") ||
                        g.getText().toString().equals("") ||
                        w.getText().toString().equals("")) {
                    result.setText("to fill in all the boxes");
                    return;
                }

                double current = Double.parseDouble(c.getText().toString());
                double goal = Double.parseDouble(g.getText().toString());
                double weight = Double.parseDouble(w.getText().toString());

                if (weight == 0) {
                    result.setText("a weight greater than 0");
                    return;
                }

                float val = (float) ((goal - (current * (1-(weight/100)))) / (weight/100));
                if (val < 0) val = 0;
                result.setText(String.valueOf(val) + "%");
            }
        });
    }
}